#include "syscall_init.h"
#include "syscall.h"
#include "thread.h"
#include "print.h"

#define SYSCALL_NR 32
typedef void* syscall_t;
syscall_t syscall_table[SYSCALL_NR];

uint32_t sys_getpid(void)
{
    return running_thread()->pid;
}

void syscall_init(void)
{
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    put_str("syscall_init done\n");
}