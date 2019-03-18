#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

// 将位图初始化
void bitmap_init(struct bitmap* btmp)
{
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

// 判断bit_idx位是否为1。若1，返回true；否则，返回false。
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    return ((btmp->bits[byte_idx] >> bit_odd) & BITMAP_MASK);
}

// 在位图中申请连续cnt个位。成功返回下标；失败返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt)
{
    // 利用状态机，遍历每个bit
    enum status
    {
        S0,                                  // 不在连续的空闲序列中。遇到空闲bit进入状态S1。
        S1                                   // 在连续的空闲序列中。遇到非空闲bit进入状态S0。
    };
    enum status s = S0;
    uint32_t copy = cnt;
    int start_idx;
    for (uint32_t i = 0; i < btmp->btmp_bytes_len; ++i)
    {
        uint8_t mask = 0x1;
        for (uint8_t j = 0; j < 8; ++j, mask <<= 1)
        {
            uint8_t is_fill = btmp->bits[i] & mask;
            switch (s)
            {
            case S0:
                if (is_fill)
                {
                }
                else
                {
                    s = S1;
                    start_idx = i * 8 + j;
                    if (--cnt == 0)
                    {
                        return start_idx;
                    }
                }
                break;
            case S1:
                if (is_fill)
                {
                    cnt = copy;
                    s = S0;
                }
                else
                {
                    if (--cnt == 0)
                    {
                        return start_idx;
                    }
                }
            }
        }
    }
    return -1;
}

// 将位图的bit_idx位设置为value
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value)
{
    ASSERT((value == 0) || (value == 1));
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd  = bit_idx % 8;
    if (value == 1)
    {
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    }
    else
    {
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
    }
}