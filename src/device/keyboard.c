#include "keyboard.h"
#include "print.h"
#include "stdint.h"
#include "interrupt.h"
#include "io.h"
#include "global.h"
#include "ioqueue.h"

// 键盘buffer寄存器端口号为0x60
#define KBD_BUF_PORT 0x60

// 转义字符
#define esc       '\33'                      // 8进制
#define backspace '\b'
#define tab       '\t'
#define enter     '\r'
#define delete    '\177'                     // 8进制

// 不可见字符一律定义为0
#define char_invisible   0
#define ctrl_l_char      char_invisible
#define ctrl_r_char      char_invisible
#define shift_l_char     char_invisible
#define shift_r_char     char_invisible
#define alt_l_char       char_invisible
#define alt_r_char       char_invisible
#define caps_lock_char   char_invisible

// 定义控制字符的通码和断码
#define shift_l_make     0x2a
#define shift_r_make     0x36
#define alt_l_make       0x38
#define alt_r_make       0xe038
#define alt_r_break      0xe0b8
#define ctrl_l_make      0x1d
#define ctrl_r_make      0xe01d
#define ctrl_r_break     0xe09d
#define caps_lock_make   0x3a

// 定义键盘缓冲队列
struct ioqueue keyboard_buffer;

// 以通码make_code为索引的二维数组，根据我的电脑的按键设置。但在我电脑上按不了ctrl
static char keymap[][2] = {
    {0, 0},
    {esc, esc},
    {'1', '!'},
    {'2', '@'},
    {'3', '#'},
    {'4', '$'},
    {'5', '%'},
    {'6', '^'},
    {'7', '&'},
    {'8', '*'},
    {'9', '('},
    {'0', ')'},
    {'-', '_'},
    {'=', '+'},
    {backspace, backspace},
    {tab, tab},
    {'q', 'Q'},
    {'w', 'W'},
    {'e', 'E'},
    {'r', 'R'},
    {'t', 'T'},
    {'y', 'Y'},
    {'u', 'U'},
    {'i', 'I'},
    {'o', 'O'},
    {'p', 'P'},
    {'[', '{'},
    {']', '}'},
    {enter, enter},
    {ctrl_l_char, ctrl_l_make},
    {'a', 'A'},
    {'s', 'S'},
    {'d', 'D'},
    {'f', 'F'},
    {'g', 'G'},
    {'h', 'H'},
    {'j', 'J'},
    {'k', 'K'},
    {'l', 'L'},
    {';', ':'},
    {'\'', '"'},
    {'`', '~'},
    {shift_l_char, shift_l_char},
    {'\\', '|'},
    {'z', 'Z'},
    {'x', 'X'},
    {'c', 'C'},
    {'v', 'V'},
    {'b', 'B'},
    {'n', 'N'},
    {'m', 'M'},
    {',', '<'},
    {'.', '>'},
    {'/', '?'},
    {shift_r_char, shift_r_char},
    {'*', '*'},
    {alt_l_char, alt_l_char},
    {' ', ' '},
    {caps_lock_char, caps_lock_char}
};

// 键盘中断处理程序
static void intr_keyboard_handler(void)
{
    // 记录相应的按键是否按下
    static bool ctrl_status, shift_status, alt_status, caps_lock_status;
    // 记录makecode是否以0xe0开头
    static bool ext_scancode;

    bool ctrl_down_last = ctrl_status;
    bool shift_down_last = shift_status;
    bool caps_lock_last = caps_lock_status;

    bool break_code;
    uint16_t scancode = inb(KBD_BUF_PORT);

    // put_int((uint32_t)scancode);
    // put_char('\n');
    // return ;

    // 以0xe0开头，表示此按键按下将产生多个扫描码
    if (scancode == 0xe0)
    {
        ext_scancode = true;
        return;
    }

    // 上次是以0xe0开头
    if (ext_scancode)
    {
        scancode |= 0xe000;
        ext_scancode = false;
    }

    // 断码的第8位为1
    break_code = ((scancode & 0x0080) != 0);

    if (break_code)
    {
        uint16_t make_code = (scancode & 0xff7f);

        if (make_code == ctrl_l_make || make_code == ctrl_r_make)
            ctrl_status = false;
        else if (make_code == shift_l_make || make_code == shift_r_make)
            shift_status = false;
        else if (make_code == alt_l_make || make_code == alt_r_make)
            alt_status = false;
        return;
    }
    // 数组中定义的键以及alt_r和ctrl_r
    else if ((scancode > 0x00 && scancode < 0x3b) || scancode == alt_r_make || scancode == ctrl_r_make)
    {
        bool shift = false;
        // 除了字母以外的
        if ((scancode <  0x0e) || (scancode == 0x29) || (scancode == 0x1a) || (scancode == 0x1b) ||
            (scancode == 0x2b) || (scancode == 0x27) || (scancode == 0x28) || (scancode == 0x33) ||
            (scancode == 0x34) || (scancode == 0x35))
        {
            if (shift_down_last)
                shift = true;
        }
        else
        {
            if (shift_down_last && caps_lock_last)
            {
                shift = false;
            }
            else if (shift_down_last || caps_lock_last)
            {
                shift = true;
            }
            else
            {
                shift = false;
            }
        }
        uint8_t index = (scancode & 0x00ff);

        char curr_char = keymap[index][shift];

        if (curr_char)
        {
            if (!ioq_full(&keyboard_buffer))
            {
                put_char(curr_char);
                ioq_putchar(&keyboard_buffer, curr_char);
            }
            return;
        }

        if (scancode == ctrl_l_make || scancode == ctrl_r_make)
        {
            ctrl_status = true;
        }
        else if (scancode == shift_l_make || scancode == shift_r_make)
        {
            shift_status = true;
        }
        else if (scancode == alt_l_make || scancode == alt_r_make)
        {
            alt_status = true;
        }
        else if (scancode == caps_lock_make)
        {
            caps_lock_status = !caps_lock_status;
        }
    }
    else
    {
        put_str("Unknown key\n");
    }

}

// 键盘初始化
void keyboard_init(void)
{
    put_str("keyboard init start\n");
    ioqueue_init(&keyboard_buffer);
    register_handler(0x21, intr_keyboard_handler);
    put_str("keyboard init done\n");
}