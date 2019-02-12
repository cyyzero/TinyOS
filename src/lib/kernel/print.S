TI_GDT equ 0
RPL0   equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

[bits 32]
section .text
; 把栈中的一个字符写入到光标所在处
global put_char
put_char:
    pushad                                   ; 备份32位寄存器环境

    mov ax, SELECTOR_VIDEO                   ; 不能直接把立即数送进段寄存器
    mov gs, ax

    ; 获取光标位置
    ; 获取高8位
    mov dx, 0x03d4                           ; 索引寄存器
    mov al, 0x0e                             ; 用于提供光标位置的高8位
    out dx, al
    mov dx, 0x03d5                           ; 通过读写数据端口0x3d5来获得或设置光标位置
    in al, dx                                ; 得到光标位置的高8位

    mov ah, al

    ; 再获得低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx

    ; 将光标存入bx
    mov bx, ax
    ; 获得需要打印的字符
    mov ecx, [esp + 36]                      ; pushad压入4*8=32字节，加上调用者的4字节返回地址

    cmp cl, 0xd                              ; 回车CR是0x0d
    jz .is_carriage_return
    cmp cl, 0xa                              ; 换行LF是0x0a
    jz .is_line_feed

    cmp cl, 0x8                              ; 退格字符是0x8
    jz .is_backspace
    jmp .put_other

.is_backspace:
    dec bx
    shl bx, 1                                ; 相当于×2，表示偏移的字节

    mov byte [gs:bx], 0x20                   ; 将待删除的字节改为空格
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    jmp .set_cursor

.put_other:
    shl bx, 1
    mov [gs:bx], cl                          ; 打印字符本身
    inc bx
    mov byte [gs:bx], 0x07                   ; 字符属性
    shr bx, 1
    inc bx                                   ; 下一个光标值
    cmp bx, 2000                             ; 若光标值小于2000，表示未写到显存的最后，
    jl .set_cursor                           ; 就设置新的光标值，否则换行处理
    jmp .roll_screen

.is_line_feed:
.is_carriage_return:
    xor dx, dx                               ; dx是被除数的高16位，清0
    mov ax, bx                               ; ax是被除数的低高16位
    mov si, 80
    div si                                   ; 得到的余数放在dx寄存器
    sub bx, dx                               ; 相当于 bx = bx - bx % 80
.is_carriage_return_end:
    add bx, 80
    cmp bx, 2000
.is_line_feed_end:
    jl .set_cursor

.roll_screen:
    cld
    mov ecx, 960                             ; 2000-80=1920个字符要复制，共1920*2=3840字节，一次搬运4字节，3840/4=960

    mov esi, 0xc00b80a0                      ; 第1行行首
    mov edi, 0xc00b8000                      ; 第0行行首
    rep movsd

    mov ebx, 3840                            ; 最后一行第一个字符
    mov ecx, 80
.cls:
    mov word [gs:ebx], 0x0720                ; 0x0720是白底黑字的空格字符
    add ebx, 2
    loop .cls
    mov bx, 1920                             ; 光标重置为最后一行的首字符

.set_cursor:
    ; 设置高8位
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh
    out dx, al

    ; 设置低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al
.put_char_done:
    popad                                    ; 恢复之前压入的寄存器
    ret

