#include "console.h"
#include "print.h"
#include "stdint.h"
#include "sync.h"
#include "thread.h"

static struct lock console_lock;

// 初始化console
void console_init(void)
{
    lock_init(&console_lock);
}

// 获取终端
void console_acquire(void)
{
    lock_acquire(&console_lock);
}

//释放终端
void console_release(void)
{
    lock_release(&console_lock);
}

// 终端输出字符
void console_put_char(uint8_t char_ascii)
{
    console_acquire();
    put_char(char_ascii);
    console_release();
}

// 终端输出16进制数字
void console_put_int(uint32_t num)
{
    console_acquire();
    put_int(num);
    console_release();
}

// 终端输出字符串
void console_put_str(const char* str)
{
    console_acquire();
    put_str(str);
    console_release();
}
