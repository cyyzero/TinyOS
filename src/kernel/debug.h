#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

void panic_spin(const char* filename, int line, const char* func, const char* condition);

#define PANIC(CONDITION) panic_spin(__FILE__, __LINE__, __func__, CONDITION)

#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0)
#else
    #define ASSERT(CONDITION) \
        if (CONDITION){ } else { PANIC(#CONDITION); }
#endif // __NDEBUG

#endif // __KERNEL_DEBUG_H