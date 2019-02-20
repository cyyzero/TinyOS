#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"

void init_all(void)
{
    put_str("init_all\n");
    idt_init();
    timer_init();
    mem_init();
    thread_init();
}