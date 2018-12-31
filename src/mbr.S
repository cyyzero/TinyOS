%include "boot.inc"
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

    mov byte [gs:0x00], 'M'
    mov byte [gs:0x01], 0x0c  ; 0表示黑色背景闪烁， c表示前景色为亮红色

    mov byte [gs:0x02], 'B'
    mov byte [gs:0x03], 0x0c

    mov byte [gs:0x04], 'R'
    mov byte [gs:0x05], 0x0c

    mov eax, LOADER_START_SECTOR       ; 所读扇区的扇区号
    mov bx, LOADER_BASE_ADDR           ; 加载到内存的目的地址
    mov cx, 1                          ; 读入的扇区数
    call rd_disk_m_16
    jmp LOADER_BASE_ADDR

; 功能： 读取硬盘
; 参数：
;     eax： 所读扇区的起始地址
;     bx ： 加载到内存的目的地址
;     cx ： 读入的扇区数
rd_disk_m_16:
    mov esi, eax
    mov di, cx

; 设置要读的扇区数
    mov dx, 0x1f2
    mov al, cl
    out dx, al

    mov eax, esi

; 将LBA地址存入0x1f3 ~ 0x1f6

    ; LBA地址0～7位写入端口0x1f3
    mov dx, 0x1f3
    out dx, al

    mov cl, 8

    ; LBA地址8~15位写入端口0x1f4
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    ; LBA地址16～23位写入端口0x1f5
    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    ; LBA地址24～27位写入端口0x1f6的低4位，高4位设置成 b1110
    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al

; 向0x1f7端口写入读命令，0x20
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

; 检测硬盘状态
.not_ready:
    nop
    in al, dx
    and al, 0x88    ; 查看第4位和第7位，分别表示已准备好数据传输和硬盘正忙
    cmp al, 0x08
    jnz .not_ready

; 从0x1f0端口读数据
    mov ax, di      ; di为读取的扇区数
    mov dx, 256
    mul dx          ; dx为读的次数，di*512/2。一个扇区512字节，每次读两个字节
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