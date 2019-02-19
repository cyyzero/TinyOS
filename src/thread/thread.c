#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096

static void kernel_thread(thread_func* function, void* func_arg)
{
    function(func_arg);
}

// 修改self_kstack以及thread_stack，并将函数指针和参数的赋值。
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
    pthread->self_kstack -= sizeof(struct intr_stack);
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

// 初始化task_struct的成员
void thread_init(struct task_struct* pthread,const char* name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic = 0x19971224;       // 自定义的魔数
}

// 创建thread并运行它
struct task_struct* thread_start(const char* name, int prio, thread_func function, void* func_arg)
{
    struct task_struct* thread = get_kernel_pages(1);
    thread_init(thread, name, prio);
    thread_create(thread, function, func_arg);

    asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" 
        : : "g" (thread->self_kstack));

    return thread;
}



