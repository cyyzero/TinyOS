#include "memory.h"
#include "stdint.h"
#include "print.h"

#define PG_SIZE 4096                         // 页框大小，4KB即4096B

#define MEM_BITMAP_BASE 0xc009a000           // 0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb。
                                             // 位图地址安排为0xc009a000。4个页框的位图，支持管理4*8*4k*4k=512MB的空间
#define K_HEAP_START 0xc0100000              // 内核堆空间起始虚拟地址。从3GB起始，跨过1MB，所以为0xc0100000

// 内存池结构
struct pool
{
    struct bitmap pool_bitmap;               // 用于管理内存的位图
    uint32_t phy_addr_start;                 // 所管理内存的起始物理地址
    uint32_t pool_size;                      // 本内存池字节容量
};
struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;

// 初始化struct pool
static void pool_init(struct pool* pool, uint8_t* btmp_bits, uint32_t btmp_len, uint32_t pool_start, uint32_t pool_size)
{
    pool->pool_bitmap.btmp_bytes_len = btmp_len;
    pool->pool_bitmap.bits = btmp_bits;
    pool->phy_addr_start = pool_start;
    pool->pool_size = pool_size;
    bitmap_init(&pool->pool_bitmap);
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

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("   mem_pool_init done\n");
}

void mem_init(void)
{
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = *((uint32_t*)0xb00);
    mem_pool_init(mem_bytes_total);
}
