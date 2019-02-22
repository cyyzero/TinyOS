#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"

void k_thread_a(void* arg);
void k_thread_b(void* arg);

void main(void)
{
    put_str("I am kernel\n");
    init_all();

    thread_start("k_thread_a", 31, k_thread_a, "argA ");
    thread_start("k_thread_b", 8,  k_thread_a, "argB ");

    intr_enable();

    for (;;)
        put_str("Main ");
}

void k_thread_a(void* arg)
{
    char* para = arg;
    for (;;)
    {
        put_str(para);
    }
}