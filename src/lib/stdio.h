#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

typedef char* va_list;

int printf(const char* fmt, ...);
int sprintf(char* buf, const char* format, ...);

#endif // __LIB_STDIO_H