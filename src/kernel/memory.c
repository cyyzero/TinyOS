#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "bitmap.h"
#include "global.h"
#include "debug.h"
#include "string.h"
#include "sync.h"
#include "thread.h"

#define MEM_BITMAP_BASE 0xc009a000           // 0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb。
                                             // 位图地址安排为0xc009a000。4个页框的位图，支持管理4*8*4k*4k=512MB的空间

#define K_HEAP_START 0xc0100000              // 内核堆空间起始虚拟地址。从3GB起始，跨过1MB，所以为0xc0100000

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)  // 得到页目录项的index
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)  // 得到页表项的index

// 内存仓库
struct arena
{
    struct mem_block_desc* desc;
    uint32_t cnt;
    bool large;
};

struct mem_block_desc k_block_descs[DESC_CNT];
struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;

// 初始化mem_block_desc
void block_desc_init(struct mem_block_desc* desc_array)
{
    for (uint16_t desc_idx = 0, block_size = 16;
         desc_idx < DESC_CNT;
         ++desc_idx, block_size *= 2)
    {
        desc_array[desc_idx].block_size = block_size;
        desc_array[desc_idx].blocks_per_arena = (PG_SIZE - sizeof(struct arena)) / block_size;
        list_init(&desc_array[desc_idx].free_list);
    }
}

// 初始化struct pool
static void pool_init(struct pool* pool, uint8_t* btmp_bits, uint32_t btmp_len, uint32_t pool_start, uint32_t pool_size)
{
    pool->pool_bitmap.bits = btmp_bits;
    pool->pool_bitmap.btmp_bytes_len = btmp_len;
    pool->phy_addr_start = pool_start;
    pool->pool_size = pool_size;
    bitmap_init(&pool->pool_bitmap);
    lock_init(&pool->lock);
}

// 在pf表示的虚拟内存池中申请pg_cnt个虚拟页
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
    int vaddr_start = 0, bit_idx_start = -1;

    if (pf == PF_KERNEL)
    {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1)
        {
            return NULL;
        }
        else
        {
            for (uint32_t cnt = 0;cnt < pg_cnt; ++cnt)
            {
                bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt, 1);
            }
            vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
        }
    }
    else
    {
        struct task_struct* curr = running_thread();

        bit_idx_start = bitmap_scan(&curr->userprog_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1)
            return NULL;

        for (uint32_t cnt = 0; cnt < pg_cnt; ++cnt)
        {
            bitmap_set(&curr->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt, 1);
        }

        vaddr_start = curr->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;

        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }
    return (void*)vaddr_start;
}

// 得到虚拟地址vaddr对应的pte指针
uint32_t* pte_ptr(void* _vaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr;
    return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
}

// 得到虚拟地址vaddr对应的pde指针
uint32_t* pde_ptr(void* _vaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr;
    // 0xfffff000指向页目录项的起始地址。高10位0xcfff定位到最后一项，为PDE的起始地址。中间10位0xcfff再定位到最后一项。指向PDE所在的页框。
    return (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
}

// 在m_pool指向的物理内存中分配一个物理页
static uint32_t palloc(struct pool* m_pool)
{
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_idx == -1)
        return 0;
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phy_addr = bit_idx * PG_SIZE + m_pool->phy_addr_start;
    return page_phy_addr;
}

// 在页表中添加虚拟地址_vaddr与物理地址_page_phy_addr的映射
static void page_table_add(void* vaddr, uint32_t page_phy_addr)
{
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);

    if (*pde & PG_P_1)                       // 页目录项存在，既有页表
    {
        ASSERT(!(*pte & PG_P_1));
        if (!(*pte & PG_P_1))
        {
            *pte = page_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
        }
        else
        {
            PANIC("pte repeat");
            *pte = page_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
        }
    }
    else                                     // 页目录项不存在，要先分配一个页表
    {
        // 分配一页作为页表，当前页表项指过去
        uint32_t pde_phy_addr = palloc(&kernel_pool);
        *pde = pde_phy_addr | PG_US_U | PG_RW_W | PG_P_1;

        memset((void*)((uint32_t)pte & 0xfffff000), 0, PG_SIZE);

        ASSERT(!(*pte & PG_P_1));
        *pte = page_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    }
}

// 分配pg_cnt个页空间，成功则返回起始虚拟地址；否则返回NULL
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);

    // 1.通过vaddr_get在虚拟内存池中申请虚拟地址
    // 2.通过palloc在物理内存中申请池
    // 3.通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射

    void* vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL)
        return NULL;

    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool* mem_pool = (pf == PF_KERNEL) ? &kernel_pool : &user_pool;

    while (cnt--)
    {
        uint32_t page_phy_addr = palloc(mem_pool);
        if (page_phy_addr == 0)
            return NULL;
        page_table_add((void*)vaddr, page_phy_addr);
        vaddr += PG_SIZE;
    }

    return vaddr_start;
}

// 从内核物理内存池中申请pg_cnt页内存，成功则返回其虚拟地址；失败则返回NULL
void* get_kernel_pages(uint32_t pg_cnt)
{
    lock_acquire(&kernel_pool.lock);
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if (vaddr != NULL)
    {
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    lock_release(&kernel_pool.lock);
    return vaddr;
}

// 在用户空间内存池中申请pg_cnt页内存，成功则返回其虚拟地址；失败则返回NULL
void* get_user_pages(uint32_t pg_cnt)
{
    lock_acquire(&user_pool.lock);
    void* vaddr = malloc_page(PF_USER, pg_cnt);
    if (vaddr != NULL)
    {
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    lock_release(&user_pool.lock);
    return vaddr;
}

// 将vaddr与pf池中的物理地址相关联，仅支持一级页空间分配
void* get_a_page(enum pool_flags pf, uint32_t vaddr)
{
    struct pool* mem_pool = (pf == PF_KERNEL) ? &kernel_pool : &user_pool;
    lock_acquire(&mem_pool->lock);

    struct task_struct* curr = running_thread();
    int32_t bit_idx = -1;

    if (curr->pgdir != NULL && pf == PF_USER)
    {
        bit_idx = (vaddr - curr->userprog_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&curr->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
    }
    else if (curr->pgdir == NULL && pf == PF_KERNEL)
    {
        bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
    }
    else
    {
        PANIC("get_a_page: not allow kernel alloc userspace or user alloc kernelspace by get_a_page");
    }

    uint32_t page_phyaddr = palloc(mem_pool);
    if (page_phyaddr == 0)
        return NULL;

    page_table_add((void*)vaddr, page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}

// 得到虚拟地址映射的物理地址
uint32_t addr_v2p(void* vaddr)
{
    uint32_t* pte = pte_ptr(vaddr);

    return ((*pte & 0xfffff000) + ((uint32_t)vaddr & 0x00000fff));
}

// 初始化内存池
static void mem_pool_init(uint32_t all_mem)
{
    put_str("   mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;                // 1页的页目录项 + 第0和第768个页目录指向同一个页表 + 
                                                             // 第769~1022个页目录项共指向254个页表。共256个页框。
    uint32_t used_mem = page_table_size + 0x100000;          // 低1MB内存
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    // 余数不作处理，会丢失一些内存，但是处理方便
    uint32_t kbm_length = kernel_free_pages / 8;                    // kernel bitmap的长度，单位为字节
    uint32_t ubm_length = user_free_pages / 8;                      // user bitmap的长度，单位为字节

    uint32_t kp_start = used_mem;                                   // 内核内存池的起始物理地址
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;     // 用户内存池起始物理地址

    pool_init(&kernel_pool, (uint8_t*)MEM_BITMAP_BASE, kbm_length, kp_start, kernel_free_pages * PG_SIZE);
    pool_init(&user_pool, (uint8_t*)MEM_BITMAP_BASE + kbm_length, ubm_length, up_start, user_free_pages * PG_SIZE);

// 输出pool信息的宏
#define PUT_POOL(level) \
put_str("   " #level "_pool_bitmap_start:"); \
put_int((int) level##_pool.pool_bitmap.bits); \
put_str("\n   " #level "_pool_phy_addr_start:"); \
put_int((int) level##_pool.phy_addr_start); \
put_char('\n');

    PUT_POOL(kernel);
    PUT_POOL(user);

    // 初始化内核虚拟地址空间的位图
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (uint8_t*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    put_str("kernel_vaddr bitmap start:");
    put_int((int)kernel_vaddr.vaddr_bitmap.bits);
    put_char('\n');

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("   mem_pool_init done\n");
}

static struct mem_block* arena2block(struct arena* a, uint32_t idx)
{
    return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + idx * a->desc->block_size);
}

// 返回内存块b所在的arena地址
static struct arena* block2arena(struct mem_block* b)
{
    return (struct arena*)((uint32_t)b & 0xfffff000);
}

void mem_init(void)
{
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = *((uint32_t*)0xb00);
    mem_pool_init(mem_bytes_total);
    block_desc_init(k_block_descs);
    put_str("mem_init finish\n");
}
