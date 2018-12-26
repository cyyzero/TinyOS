SECTION MBR vstart=0x7c00
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov sp, 0x7c00
    mov ax, 0xb800
    mov gs, ax

; INT 0x10  功能号0x06  功能为上卷窗口，即清屏
; AH 功能号
; BH 上卷行属性
; (CL, CH)  窗口左上角的坐标
; (DL, DH)  窗口右下角的坐标
; 无返回值

    mov ax, 0x600  ; 功能号6 + 上卷行数0(表示全部行)
    mov bx, 0x700  ; 上卷行属性
    mov cx, 0      ; 左上角 (0, 0)
    mov dx, 0x184f ; 右上角 (79, 24)

    int 0x10

    mov byte [gs:0x00], 'H'
    mov byte [gs:0x01], 0xA4  ; A表示绿色背景闪烁， 4表示前景色为红色

    mov byte [gs:0x02], 'e'
    mov byte [gs:0x03], 0xA4

    mov byte [gs:0x04], 'l'
    mov byte [gs:0x05], 0xA4

    mov byte [gs:0x06], 'l'
    mov byte [gs:0x07], 0xA4

    mov byte [gs:0x08], 'o'
    mov byte [gs:0x09], 0xA4

    mov byte [gs:0x0a], ' '
    mov byte [gs:0x0b], 0xA4

    mov byte [gs:0x0c], 'w'
    mov byte [gs:0x0d], 0xA4

    mov byte [gs:0x0e], 'o'
    mov byte [gs:0x0f], 0xA4

    mov byte [gs:0x10], 'r'
    mov byte [gs:0x11], 0xA4

    mov byte [gs:0x12], 'l'
    mov byte [gs:0x13], 0xA4

    mov byte [gs:0x14], 'd'
    mov byte [gs:0x15], 0xA4

    mov byte [gs:0x16], '!'
    mov byte [gs:0x17], 0xA4

    mov eax, LOADER_START_SECTOR
    mov bx, LOADER_BASE_ADDR
    mov cx, 1
    call rd_disk_m_16

    jmp LOADER_BASE_ADDR

    mov esi, eax
    mov di, cx

    mov dx, 0x1f2
    mov al, cl
    out dx, al

    mov eax, esi

    mov dx, 0x1f3
    out dx, al

    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al

    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

.not_ready:
    nop
    in al, dx
    and al, 0x88
    cmp al, 0x88
    cmp al, 0x08
    jnz .not_ready

    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax
    mov dx, 0x1f0

.go_on_read:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .go_on_read
    ret

    times 510-($-$$) db 0
    db 0x55, 0xaa