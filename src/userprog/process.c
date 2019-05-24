#include "process.h"
#include "thread.h"
#include "memory.h"
#include "debug.h"
#include "tss.h"
#include "console.h"

extern void intr_exit(void);

void start_process(void* filename_)
{
    void *function = filename_;
    struct task_struct* curr = running_thread();
    curr->self_kstack += sizeof(struct thread_stack);
    struct intr_stack* proc_stack = (struct intr_stack*)curr->self_kstack;

    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy
        = proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = get_a_page(PF_USER, USER_STACK3_VADDR + PG_SIZE);
    proc_stack->ss = SELECTOR_U_DATA;
    asm volatile("movl %0, %%esp; jmp intr_exit" : : "g" (proc_stack) : "memory");
}

// 激活页表
void page_dir_activate(struct stack_struct* pthread)
{
    // 默认是内核线程，内核线程的页目录地址为0x100000
    uint32_t pagedir_phy_addr = 0x100000;
    // 如果是进程，pagedir_phy_addr为进程的页目录项地址
    if (pthread->pgdir != NULL)
    {
        pagedir_phy_addr = addr_v2p(pthread->pgdir);
    }

    // 更新寄存器cr3
    asm volatile ("movl, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

// 激活线程或进程的页表，更新tss中的esp0为进程的特权级0的栈
void process_activate(struct task_struct* pthread)
{
    ASSERT(pthread != NULL);
    page_dir_activate(pthread);

    if (pthread->pgdir)
    {
        update_tss_esp(pthread);
    }
}

uint32_t* create_page_dir(void)
{
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    if (page_dir_vaddr == NULL)
    {
        console_put_str("create_page_dir: get_kernel_page failed!");
        return NULL;
    }

    memcpy((void*)((uint32_t)page_dir_vaddr + 0x300*4), (void*)(0xfffff000 + 0x300*4), 1024);

    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t))

}