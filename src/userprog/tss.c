#include "tss.h"
#include "stdint.h"
#include "global.h"
#include "string.h"
#include "print.h"
#include "thread.h"

// 任务状态段tss结构
struct tss
{
    uint32_t backlink;
    uint32_t* esp0;
    uint32_t ss0;
    uint32_t* esp1;
    uint32_t ss1;
    uint32_t* esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip)(void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trance;
    uint32_t io_base;
};

static struct tss tss;

void update_tss_esp(struct task_struct* pthread)
{
    tss.esp0 = (uint32_t*)((uint32_t)pthread + PG_SIZE);
}

static struct gdt_desc make_gdt_desc(uint32_t* desc_addr, uint32_t limit, uint8_t addr_low, uint8_t attr_high)
{
    uint32_t desc_base = (uint32_t)desc_addr;
    struct gdt_desc desc;
    desc.limit_low_word = limit & 0x0000ffff;
    desc.base_low_word  = desc_base & 0x0000ffff;
    desc.base_mid_byte  = (desc_base & 0x00ff0000) >> 16;
}