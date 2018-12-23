SECTION MBR vstart=0x7c00

    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov sp, 0x7c00

; INT 0x10  功能号0x06  功能为上卷窗口，即清屏
; AH 功能号
; BH 上卷行属性
; (CL, CH)  窗口左上角的坐标
; (DL, DH)  窗口右下角的坐标
; 无返回值

    mov ax, 0x600
    mov bx, 0x700
    mov cx, 0
    mov dx, 0x184f

    int 0x10

    mov ah, 3
    mov bh, 0

    int 0x10

; 0x10 打印字符串
    mov ax, message
    mov bp, ax
    mov cx, 11
    mov ax, 0x1301
    mov bx, 0x2

    int 0x10

    jmp $

    message db "Hello world"
    times 500-($-$$) db 0
    db 0x55, 0xaa