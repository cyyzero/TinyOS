#include "print.h"
#include "stdint.h"

// 打印字符串
void put_str(const char* str)
{
    for (const char* p = str; *p; ++p)
        put_char(*p);
}

// 打印十六进制数字
void put_int(uint32_t num)
{
    uint8_t flag = 0;
    for (uint32_t mask = 0xf0000000, i = 1; i <= 8; ++i, mask >>= 4)
    {
        uint8_t ch = (num & mask) >> (32- i*4);
        if (ch <= 9)
            ch += '0';
        else
            ch = 'A' + ch - 10;
        if (flag == 0 && ch != '0')
        {
            put_char(ch);
            flag = 1;
        }
        else if (flag == 1)
        {
            put_char(ch);
        }
    }
    if (flag == 0)
    {
        put_char('0');
    }
}