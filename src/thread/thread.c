#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "debug.h"
#include "list.h"
#include "interrupt.h"
#include "print.h"

#define PG_SIZE 4096

struct task_struct* main_thread;             // 主线程PCB
struct list thread_ready_list;               // 就绪队列
struct list thread_all_list;                 // 所有任务队列
static struct list_elem* thread_tag;         // 用于保存队列中的线程节点

extern void switch_to(struct task_struct* curr, struct task_struct* next);

// 获取当前线程PCB指针
struct task_struct* running_thread(void)
{
    uint32_t esp;
    asm ("mov %%esp, %0" : "=g" (esp));
    return (struct task_struct*)(esp & 0xfffff000);
}

// 由kernel_thread去执行function
static void kernel_thread(thread_func* function, void* func_arg)
{
    // 打开中断，防止后面的中断被屏蔽，而无法调度其它进程
    intr_enable();
    function(func_arg);
}

// 修改self_kstack以及thread_stack，并将函数指针和参数的赋值。
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
    // pthread->self_kstack -= sizeof(struct intr_stack);
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

// 初始化task_struct的成员
void init_thread(struct task_struct* pthread,const char* name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->status = (pthread == main_thread) ? TASK_RUNNING : TASK_READY;
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->stack_magic = 0x19971224;       // 自定义的魔数
}

// 创建thread并运行它
struct task_struct* thread_start(const char* name, int prio, thread_func function, void* func_arg)
{
    struct task_struct* thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    // 加入就绪队列
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    // 加入所有线程的队列
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    return thread;
}

// 将kernel中的main函数完善为主线程
static void make_main_thread(void)
{
    // 在loader.S中进入内核时mov esp, 0xc009f000，预留了PCB，起始地址为0xc009e000
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

// 实现任务调度
void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct* curr = running_thread();
    if (curr->status == TASK_RUNNING)
    {
        ASSERT(!elem_find(&thread_ready_list, &curr->general_tag));
        list_append(&thread_ready_list, &curr->general_tag);
        curr->ticks = curr->priority;
        curr->status = TASK_READY;
    }
    else
    {
        // 若此线程需要某些事件发生之后才能继续在cpu上运行，不需要将其加入队列，因为它不在就绪队列中
    }

    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(curr, next);
}

// 阻塞当前线程
void thread_block(enum task_status status)
{
    ASSERT((status == TASK_BLOCKED) || (status == TASK_WAITING) || (status == TASK_HANGING));

    enum intr_status old_status = intr_disable();
    struct task_struct* curr_thread = running_thread();
    curr_thread->status = status;
    schedule();
    intr_set_status(old_status);
}

// 当前线程解除阻塞
void thread_unblock(struct task_struct* pthread)
{
    enum intr_status old_status = intr_disable();
    ASSERT(pthread->status == TASK_BLOCKED || pthread->status == TASK_WAITING || pthread->status == TASK_HANGING);
    if (pthread->status != TASK_READY)
    {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if (elem_find(&thread_ready_list, &pthread->general_tag))
        {
            PANIC("thread_unblock: blocked thread in ready_list\n");
        }
        // 放到队列首部，尽快调度
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}

void thread_init(void)
{
    put_str("thread_init start\n");

    list_init(&thread_all_list);
    list_init(&thread_ready_list);

    make_main_thread();

    put_str("thread_init done\n");
}
