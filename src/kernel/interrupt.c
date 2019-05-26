#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "print.h"
#include "io.h"

#define PIC_M_CTRL 0x20                      // 主片控制端口为0x20
#define PIC_M_DATA 0x21                      // 主片数据端口为0x21
#define PIC_S_CTRL 0xa0                      // 从片控制端口为0xa0
#define PIC_S_DATA 0xa1                      // 从片数据端口为0xa1

#define IDT_DESC_CNT 0x30

#define EFLAGS_IF 0x00000200                 // eflags寄存器中的if位为1
#define GET_EFLAGS(EFLAG_VAR) asm volatile ("pushfl; popl %0" : "=g" (EFLAG_VAR))

// 中断门描述符
struct gate_desc
{
    uint16_t func_offset_low_word;           // 0..15位的偏移量
    uint16_t selector;                       // 代码段选择子
    uint8_t  dcount;                         // 
    uint8_t  attribute;                      // 属性
    uint16_t func_offset_high_word;          // 16..31位的偏移量
};

// 定义在kernel.S中的中断处理函数入口数组
extern intr_handler intr_entry_table[IDT_DESC_CNT];

// 中断描述符表
static struct gate_desc idt[IDT_DESC_CNT];

// 用于保存异常的名字
const char* intr_name[IDT_DESC_CNT];

// 定义中断处理程序数组
intr_handler idt_table[IDT_DESC_CNT];

// 创建中段门描述符
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

// 初始化中断描述符表
static void idt_desc_init(void)
{
    for (int i = 0; i < IDT_DESC_CNT; ++i)
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    put_str("   idt_desc_init done\n");
}

// 初始化可编程中断控制器8259A
static void pic_init(void)
{
    // 初始化主片
    outb(PIC_M_CTRL, 0x11);                  // ICW1: 边沿触发，级联8259，需要ICW4
    outb(PIC_M_DATA, 0x20);                  // ICW2: 起初中断向量号为0x20

    outb(PIC_M_DATA, 0x04);                  // ICW3: IR2接从片
    outb(PIC_M_DATA, 0x01);                  // ICW4: 8086模式，正常EOI

    // 初始化从片
    outb(PIC_S_CTRL, 0x11);                  // ICW1: 边沿触发，级联8259，需要ICW4
    outb(PIC_S_DATA, 0x28);                  // ICW2: 起始中断向量号为0x28

    outb(PIC_S_DATA, 0x02);                  // ICW3: 设置从片连接到主片的IR2引脚
    outb(PIC_S_DATA, 0x01);                  // ICW4: 8086模式，正常EOI

    // OCW1 打开主片上IR0，目前只接受时钟产生的中断，以及键盘的中断
    outb(PIC_M_DATA, 0xfc);
    outb(PIC_S_DATA, 0xff);

    put_str("   pic_init done\n");
}

// 通用的中断处理程序，一般在异常出现时处理
static void general_intr_handler(uint8_t vec_nr)
{
    // IRQ7和IRQ15会产生伪中断(spurious interrupt)，无需处理
    if (vec_nr == 0x27 || vec_nr == 0x2f)
        return;

    // set_cursor定义在print.S中
    set_cursor(0);
    for (int cursor_pos = 0; cursor_pos < 320; ++cursor_pos)
    {
        put_char(' ');
    }
    set_cursor(0);
    put_str("!!!!!      exception message begin      !!!!!\n");
    set_cursor(88);                          // 第二行第八个字符
    put_str(intr_name[vec_nr]);
    if (vec_nr == 14)                        // page fault
    {
        // cr2中存放造成page_fault的地址。放进page_fault_vaddr
        uint32_t page_fault_vaddr = 0;
        asm ("movl %%cr2, %0" : "=r" (page_fault_vaddr));
        put_str("\npage fault addr is ");
        put_int(page_fault_vaddr);
    }

    put_str("!!!!!      exception message end      !!!!!\n");

    // 能进入中断处理程序就表示已经处在关中断的情况下。下面的死循环不会被中断。
    for (;;)
        ;
}

// 初始化中断向量和中断名表
static void exception_init(void)
{
    for (int i = 0; i < IDT_DESC_CNT; ++i)
    {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }

    intr_name[0]  = "#DE Divide Error";
    intr_name[1]  = "#DB Debug Exception";
    intr_name[2]  = "NMI Interrupt";
    intr_name[3]  = "#BP Breakpoint Exception";
    intr_name[4]  = "#OF Overflow Exception";
    intr_name[5]  = "#BR BOUND Range Exceeded Exception";
    intr_name[6]  = "#UD Invalid Opcode Exception";
    intr_name[7]  = "#NM Device Not Available Exception";
    intr_name[8]  = "#DF Double Fault Exception";
    intr_name[9]  = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

// 开中断，并返回开中断前的状态
enum intr_status intr_enable(void)
{
    enum intr_status old_status;
    if (intr_get_status() == INTR_ON)
    {
        old_status = INTR_ON;
    }
    else
    {
        old_status = INTR_OFF;
        asm volatile("sti");                 // 开中断，sti指令将IF位置1
    }
    return old_status;
}

// 关中断，并返回关中断前的状态
enum intr_status intr_disable(void)
{
    enum intr_status old_status;
    if (intr_get_status() == INTR_ON)
    {
        old_status = INTR_ON;
        asm volatile("cli" : : : "memory");  // 关中断，cli指令将IF位置0
    }
    else
    {
        old_status = INTR_OFF;
    }
    return old_status;
}

// 设置中断状态
enum intr_status intr_set_status(enum intr_status status)
{
    return (status == INTR_ON) ? intr_enable() : intr_disable();
}

// 获取当前中断状态
enum intr_status intr_get_status(void)
{
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}

// 注册中断处理函数
void register_handler(uint8_t vector_no, intr_handler function)
{
    idt_table[vector_no] = function;
}

// 完成中断相关的初始化工作
void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init();                         // 初始化中段描述符表
    exception_init();
    pic_init();                              // 初始化8259A

    // 加载IDT
    uint64_t idt_operand = (sizeof(idt) - 1) | ((uint64_t)idt << 16);
    asm volatile("lidt %0" : : "m" (idt_operand));
    put_str("idt_init done\n");
}
