#include "process.h"
#include "thread.h"
#include "memory.h"
#include "bitmap.h"
#include "debug.h"
#include "interrupt.h"
#include "tss.h"
#include "console.h"
#include "string.h"

extern void intr_exit(void);

// 创建用户进程上下文信息
void start_process(void *filename_)
{
    void *function = filename_;
    struct task_struct *curr = running_thread();
    curr->self_kstack += sizeof(struct thread_stack);
    struct intr_stack *proc_stack = (struct intr_stack *)curr->self_kstack;

    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy
        = proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE;
    proc_stack->ss = SELECTOR_U_DATA;
    asm volatile("movl %0, %%esp; jmp intr_exit"
                 :
                 : "g"(proc_stack)
                 : "memory");
}

// 激活页表
void page_dir_activate(struct task_struct *pthread)
{
    // 默认是内核线程，内核线程的页目录地址为0x100000
    // 如果是进程，pagedir_phy_addr为进程的页目录项地址
    uint32_t pagedir_phy_addr = (pthread->pgdir != NULL) ? addr_v2p(pthread->pgdir) : 0x100000;

    // 更新寄存器cr3
    asm volatile("movl %0, %%cr3"
                 :
                 : "r"(pagedir_phy_addr)
                 : "memory");
}

// 激活线程或进程的页表，更新tss中的esp0为进程的特权级0的栈
void process_activate(struct task_struct *pthread)
{
    ASSERT(pthread != NULL);
    page_dir_activate(pthread);

    // 内核线程不需要更新esp0
    if (pthread->pgdir != NULL)
    {
        update_tss_esp(pthread);
    }
}

// 创建页目录表，将内核空间的pde赋值。成功返回页目录的虚拟地址；失败返回-1
uint32_t *create_page_dir(void)
{
    uint32_t *page_dir_vaddr = get_kernel_pages(1);
    if (page_dir_vaddr == NULL)
    {
        console_put_str("create_page_dir: get_kernel_page failed!");
        return NULL;
    }

    // 复制内核页目录项第768项起始到最后
    memcpy((void *)((uint32_t)page_dir_vaddr + 0x300 * 4), (void *)(0xfffff000 + 0x300 * 4), 1024);
    // 更新最后一项
    uint32_t new_page_dir_phy_addr = addr_v2p(page_dir_vaddr);
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;

    return page_dir_vaddr;
}

// 创建用户进程虚拟地址位图
void create_user_vaddr_bitmap(struct task_struct *user_prog)
{
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

// 创建用户进程
void process_execute(void *filename, const char *name)
{
    struct task_struct *thread = get_kernel_pages(1);
    init_thread(thread, name, DEFAULT_PRIO);
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process, filename);
    thread->pgdir = create_page_dir();

    enum intr_status old_status = intr_disable();

    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    intr_set_status(old_status);
}
