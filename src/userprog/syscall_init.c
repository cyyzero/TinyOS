#include "syscall_init.h"
#include "syscall.h"
#include "thread.h"
#include "print.h"
#include "console.h"
#include "string.h"

#define SYSCALL_NR 32
typedef void* syscall_t;
syscall_t syscall_table[SYSCALL_NR];

uint32_t sys_getpid(void)
{
    return running_thread()->pid;
}

uint32_t sys_write(const char* str)
{
    console_put_str(str);
    return strlen(str);
}

void syscall_init(void)
{
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    put_str("syscall_init done\n");
}