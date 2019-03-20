#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#include "stdint.h"

// GDT描述符属性
#define DESC_G_4k      1
#define DESC_D_32      1
#define DESC_L         0
#define DESC_AVL       0
#define DESC_P         1
#define DESC_DPL_0     0
#define DESC_DPL_1     1
#define DESC_DPL_2     2
#define DESC_DPL_3     3
// 代码段和数据段属于存储段，tss和各种门描述符属于系统段。S位为1表示存储段，0表示系统段
#define DESC_S_CODE    1
#define DESC_S_DATA    DESC_S_CODE
#define DESC_S_SYS     0
#define DESC_TYPE_CODE 8                     // 1000，代码段是可执行、 非依从、不可读、已访问位a清零
#define DESC_TYPE_DATA 2                     // 0010，数据段是不可执行、向上扩展、可写、 已访问位a清零
#define DESC_TYPE_TSS  9                     // 1001

#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define TI_GDT 0
#define TI_LDT 1

#define SELECTOR_K_CODE  ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA  ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK SELECTOR_K_DATA
#define SELECTOR_K_GS    ((3 << 3) + (TI_GDT << 2) + RPL0)
// 第3个段描述符是显存；第4个是tss
#define SELECTOR_U_CODE  ((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA  ((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_STACK SELECTOR_U_DATA

#define GDT_ATTR_HIGH                ((DESC_G_4k << 7) + (DESC_D_32 << 6) + (DESC_L << 5) + (DESC_AVL << 4))
#define GDT_CODE_ATTR_LOW_DPL3       ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_CODE << 4) + DESC_TYPE_CODE)
#define GDT_DATA_ATTR_LOW_DPL3       ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_DATA << 4) + DESC_TYPE_DATA)

// TSS描述符属性
#define TSS_DESC_D    0
#define TSS_ATTR_HIGH ((DESC_G_4k << 7) + (TSS_DESC_D << 6) + (DESC_L << 5) + (DESC_AVL << 4) + 0x0)
#define TSS_ATTR_LOW  ((DESC_P << 7) + (DESC_DPL_0 << 5) + (DESC_S_SYS << 4) + DESC_TYPE_TSS)
#define SELECTOR_TSS  ((4 << 3) + (TI_GDT << 2) + RPL0)

struct gdt_desc
{
    uint16_t limit_low_word;
    uint16_t base_low_word;
    uint8_t  base_mid_byte;
    uint8_t  attr_low_byte;
    uint8_t  limit_high_attr_high;
    uint8_t  base_high_byte;
};

// IDT 描述符属性
#define IDT_DESC_P         1                 // present
#define IDT_DESC_DPL0      0                 // DPL
#define IDT_DESC_DPL3      3
#define IDT_DESC_S         0                 // Storage Segment，中断门设置为0
#define IDT_DESC_32_TYPE   0xE               // 80386 32-bit interrupt gate
#define IDT_DESC_16_TYPE   0x6               // 80286 16-bit interrupt gate

// DPL为0的中段描述符的属性字段
#define IDT_DESC_ATTR_DPL0 ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + (IDT_DESC_S << 4) + IDT_DESC_32_TYPE)
// DPL为3的中段描述符的属性字段
#define IDT_DESC_ATTR_DPL3 ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + (IDT_DESC_S << 4) + IDT_DESC_32_TYPE)

#define EFLAGS_MBS    (1 << 1)
#define EFLAGS_IF_1   (1 << 9)
#define EFLAGS_IF_0   (0 << 9)
#define EFLAGS_IOPL_3 (3 << 12)
#define EFLAGS_IPOL_0 (0 << 12)

#define PG_SIZE 4096

#define NULL  ((void*)0)
#define bool  _Bool
#define true  1
#define false 0

#endif // __KERNEL_GLOBAL_H

