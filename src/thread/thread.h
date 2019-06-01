#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "stdint.h"
#include "list.h"
#include "memory.h"

typedef int16_t pid_t;
typedef void thread_func(void*);

// 进程或线程的状态
enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

// 中断栈，用于中断发生时保护程序的上下文。与kernel.S定义的压入栈的顺序对应。
struct intr_stack
{
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code;
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

// 线程栈，用于存储线程中执行的函数
struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    // 线程第一次执行时，eip指向线程函数；之后，eip指向switch_to返回地址
    void (*eip)(thread_func* func, void* func_arg);

    // 以下仅供第一次被调度上cpu时使用
    void *unused_retaddr;
    thread_func* function;
    void* func_arg;
};

// PCB，进程或者线程的控制块
struct task_struct
{
    uint32_t* self_kstack;                   // 内核线程自己的内核栈
    pid_t pid;
    enum task_status status;
    char name[16];

    uint8_t priority;
    uint8_t ticks;                           // 每次在处理器上执行的时间滴答数
    uint32_t elapsed_ticks;                  // 此任务自上cpu运行后至今占用了多少cpu滴答数

    struct list_elem general_tag;            // 用于表示线程在一般的队列中的节点
    struct list_elem all_list_tag;           // 用于表示线程队列thread_all_list中的节点

    uint32_t* pgdir;                         // 进程自己页表的虚拟地址
    struct virtual_addr userprog_vaddr;      // 用户进程的虚拟地址
    uint32_t stack_magic;                    // 栈的边界标记，用于检测栈的溢出
};

extern struct list thread_ready_list;               // 就绪队列
extern struct list thread_all_list;                 // 所有任务队列

struct task_struct* running_thread(void);
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void init_thread(struct task_struct* pthread,const char* name, int prio);
struct task_struct* thread_start(const char* name, int prio, thread_func function, void* func_arg);
void schedule(void);
void switch_to(struct task_struct* curr, struct task_struct* next);

void thread_block(enum task_status status);
void thread_unblock(struct task_struct* pthread);

void thread_init(void);

#endif // __THREAD_THREAD_H
