#include "debug.h"
#include "print.h"
#include "interrupt.h"

void panic_spin(const char* filename, int line, const char* func, const char* condition)
{
    intr_disable();

    put_str("\n\n\n!!!!! error !!!!!\n");

    put_str("filename:");
    put_str(filename);
    put_char('\n');

    put_str("line:0x");
    put_int(line);
    put_char('\n');

    put_str("function:");
    put_str(func);
    put_str("\n");

    put_str("condtion:");
    put_str(condition);
    put_char('\n');
    for (;;)
        ;
}