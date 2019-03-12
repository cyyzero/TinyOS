#ifndef __USERPROG_TSS_H
#define __USERPROG_TSS_H

#include "stdint.h"

struct task_struct;
struct gdt_desc;

void update_tss_esp(struct task_struct* pthread);
void make_gdt_desc(struct gdt_desc* desc, uint32_t desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high);
void tss_init(void);

#endif // __USERPROG_TSS_H