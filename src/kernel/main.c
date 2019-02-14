#include "print.h"
#include "init.h"
#include "debug.h"

void main(void)
{
    put_str("I am kernel\n");
    init_all();
    ASSERT(1==2);
    for (;;)
        ;
}