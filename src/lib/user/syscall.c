#include "syscall.h"

#define _SYSCALL0(NUMBER) ({ \
    int retval;              \
    asm volatile(            \
        "int $0x80"          \
        : "=a"(retval)       \
        : "a"(NUMBER)        \
        : "memory");         \
    retval;                  \
})

#define _SYSCALL1(NUMBER, ARG1) ({ \
    int retval;                    \
    asm volatile(                  \
        "int $0x80"                \
        : "=a"(retval)             \
        : "a"(NUMBER), "b"(ARG1)   \
        : "memory");               \
    retval;                        \
})

#define _SYSCALL2(NUMBER, ARG1, ARG2) ({    \
    int retval;                             \
    asm volatile(                           \
        "int $0x80"                         \
        : "=a"(retval)                      \
        : "a"(NUMBER), "b"(ARG1), "c"(ARG2) \
        : "memory");                        \
    retval;                                 \
})

#define _SYSCALL3(NUMBER, ARG1, ARG2, ARG3) ({         \
    int retval;                                        \
    asm volatile(                                      \
        "int $0x80"                                    \
        : "=a"(retval)                                 \
        : "a"(NUMBER), "b"(ARG1), "c"(ARG2), "d"(ARG3) \
        : "memory");                                   \
    retval;                                            \
})

uint32_t getpid(void)
{
    return _SYSCALL0(SYS_GETPID);
}

uint32_t write(const char* str)
{
    return _SYSCALL1(SYS_WRITE, str);
}
