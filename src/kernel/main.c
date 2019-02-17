#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"

void main(void)
{
    put_str("I am kernel\n");
    init_all();

    void* addr = get_kernel_pages(3);
    put_str("\n get_kernel_page start vaddr is ");
    put_int((int)addr);
    put_char('\n');

    for (;;)
        ;
}