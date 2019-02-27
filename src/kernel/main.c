#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"

void k_thread_a(void* arg);
void k_thread_b(void* arg);

void main(void)
{
    put_str("I am kernel\n");
    init_all();

    thread_start("consumer_a", 31, k_thread_a, " A_");
    thread_start("consumer_b", 31, k_thread_a, " B_");

    intr_enable();

    for (;;);
        // console_put_str("Main ");
}

void k_thread_a(void* arg)
{
    for (;;)
    {
        enum intr_status old_status = intr_disable();
        if (!ioq_empty(&keyboard_buffer))
        {
            console_put_str((const char*)arg);
            char byte = ioq_getchar(&keyboard_buffer);
            console_put_char(byte);
        }
        intr_set_status(old_status);
    }
}