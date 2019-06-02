#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "process.h"
#include "syscall.h"
#include "syscall_init.h"
#include "stdio.h"

void k_thread_a(void* arg);
void k_thread_b(void* arg);
void u_prog_a(void);
void u_prog_b(void);

static int prog_a_pid, prog_b_pid;

static int test_var_a = 0, test_var_b = 0;

void main(void)
{
    put_str("I am kernel\n");
    init_all();

    process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");

    intr_enable();
    console_put_str(" main_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    thread_start("k_thread_a", 31, k_thread_a, "argA ");
    thread_start("k_thread_b", 31, k_thread_b, "argB ");

    for (;;);
        // console_put_str("Main ");
}

void k_thread_a(void* arg)
{
    char* para = arg;
    console_put_str(" thread_a_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    for (;;)
        ;
}

void k_thread_b(void* arg)
{
    char* para = arg;
    console_put_str(" thread_b_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    for (;;)
        ;
}

void u_prog_a(void)
{
    printf(" I am %s, my pid:%d%c", "prog_a", getpid(), '\n');
    for (;;)
        ;
}

void u_prog_b(void)
{
    printf(" I am %s, my pid:%d%c", "prog_b", getpid(), '\n');
    printf(" The minnum int32_t is %d\n", -0x7fffffff-1);
    printf("%d\n", -1);
    for (;;)
        ;
}
