#include "print.h"
#include "init.h"
#include "thread.h"

void k_thread_a(void* arg);

void main(void)
{
    put_str("I am kernel\n");
    init_all();

    thread_start("k_thread_a", 31, k_thread_a, "argA ");

    for (;;)
        ;
}

void k_thread_a(void* arg)
{
    char* para = arg;
    for (;;)
    {
        put_str(para);
    }
}