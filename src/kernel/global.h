#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#include "stdint.h"

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

#endif // __KERNEL_GLOBAL_H
