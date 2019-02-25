#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H

#include "stdint.h"

void console_init(void);
void console_acquire(void);
void console_release(void);
void console_put_char(uint8_t char_asci);
void console_put_int(uint32_t num);
void console_put_str(const char* str);

#endif // __DEVICE_CONSOLE_H
