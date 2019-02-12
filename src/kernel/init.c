#include "init.h"
#include "print.h"
#include "interupt.h"

void init_all(void)
{
    idt_init();
    put_str("init_all\n");
}