#include "timer.h"
#include "io.h"
#include "print.h"

#define IRQ0_FREQUENCY   100
#define INPUT_FREQUENCY  1193180
#define COUNTER0_VALUE   INPUT_FREQUENCY/IRQ0_FREQUENCY
#define COUNTER0_PORT    0x40                                // 寄存器0的控制端口：0x40
#define COUNTER0_NO      0                                   // 选择计数器：选择计数器0
#define COUNTER_MODE     2                                   // 工作模式：方式2，N分频
#define READ_WRITE_LATCH 3                                   // 读写方式：先读写低字节，再读写高字节
#define PIT_CONTROL_PORT 0x43                                // 控制字寄存器端口为0x43

// 把操作的计数器counter_no、读写锁属性rwl、计数器模式counter_mode写入模式控制寄存器并赋予初值counter_value
static void frequency_set(uint8_t counter_port, uint8_t counter_no, uint8_t rwl, uint8_t counter_mode, uint16_t counter_value)
{
    // 往控制字寄存器端口0x43写入控制字
    outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    // 先写入counter_value的低8位
    outb(counter_port, (uint8_t)counter_value);
    // 后写入counter_value的高8位
    outb(counter_port, (uint8_t)(counter_value >> 8));
}

// 初始化时钟，修改时钟频率为100Hz
void timer_init(void)
{
    put_str("timer_init start\n");
    frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
    put_str("timer_init done\n");
}