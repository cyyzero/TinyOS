#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "stdint.h"

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

// 中断栈，用于中断发生时保护程序的上下文
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
    enum task_status status;
    uint8_t priority;
    char name[16];
    uint32_t stack_magic;                    // 栈的边界标记，用于检测栈的溢出
};

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void thread_init(struct task_struct* pthread,const char* name, int prio);
struct task_struct* thread_start(const char* name, int prio, thread_func function, void* func_arg);

#endif // __THREAD_THREAD_H