%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

SELECTOR_CODE  equ (0x0001 << 3) + TI_GDT + RPL0
SELECTOR_DATA  equ (0x0002 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

; 构建gdt及内部的描述符
GDT_BASE:        dd 0x00000000
                 dd 0x00000000

CODE_DESC:       dd 0x0000FFFF
                 dd DESC_CODE_HIGH4

DATA_STACK_DESC: dd 0x0000FFFF
                 dd DESC_DATA_HIGH4

VIDEO_DESC:      dd 0x80000007         ; limit = (0xbffff-0xb8000)/4k = 0x7
                 dd DESC_VIDEO_HIGH4
times 60 dq 0
GDT_SIZE   equ $ - GDT_BASE
GDT_LIMIT  equ GDT_SIZE - 1

; 保存内存容量，以字节为单位。地址为0x900+0x200=0xb00
total_mem_bytes dd 0

gdt_ptr dw GDT_LIMIT
        dd GDT_BASE

; 人工对齐，total_mem_bytes(4) + gdt_ptr(6) + ards_buf(244) + ards_nr(2) = 256 bytes = 0x100 bytes
ards_buf times 244 db 0                ; RAM大小信息的数组
ards_nr dw 0                           ; 数组的大小

loader_start:

; int 15h 中断
; eax = 0000E820h, edx = 534D4150h ('SMAP')获取内存布局, ebx = 0
; reference: http://www.uruk.org/orig-grub/mem64mb.html
    xor ebx, ebx                       ; 第一次调用，ebx需要清零
    mov edx, 0x534d4150                ; ‘SMAP’
    mov di, ards_buf                   ; es:di指向缓冲区，存放得到的数据
.e820_mem_get_loop:
    mov eax, 0x0000e820
    mov ecx, 20                        ; 缓冲区大小
    int 0x15
    jc .e820_failed_so_try_e801;       ; CF置1为调用失败
    add di, cx                         ; 更新es:di
    inc word [ards_nr]
    cmp ebx, 0                         ; CF不为1且ebx为0,表示结束
    jnz .e820_mem_get_loop

    ; 获取数组中 base_add_low + length_low 最大的元素，即内存的容量
    mov cx, [ards_nr]
    mov ebx, ards_buf
    xor edx, edx               ; edx保存内存的容量
.find_max_mem_area:
    mov eax, [ebx]
    add eax, [ebx+8]
    add ebx, 20
    cmp edx, eax
    jge .next_ards
    mov edx, eax
.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok


; int 15h 中断，最大支持4g
; ax = E801h
.e820_failed_so_try_e801:
    mov ax, 0xe801
    int 0x15
    jc .e801_failed_so_try_88

    ; 计算出低15MB的内存
    ; ax, cx中为内存大小，单位为KB
    mov cx, 0x400
    mul cx
    shl edx, 16
    and eax, 0x0000ffff
    or edx, eax
    add edx, 0x100000                  ; 增加1MB
    mov esi, edx

    ; 计算出16MB以上的内存
    ; 寄存器bx和dx中是以64KB为单位的内存数量
    xor eax, eax
    mov ax, bx
    mov ecx, 0x10000
    mul ecx

    add esi, eax
    mov edx, esi
    jmp .mem_get_ok


; int 15h 中断
; ah = 0x88，只能获取64MB以内的RAM大小
.e801_failed_so_try_88:
    mov ah, 0x88
    int 0x15
    jc .error_hlt
    and eax, 0x0000ffff

    mov cx, 0x400
    mul cx
    shl edx, 16
    or edx, eax
    add edx, 0x100000                  ; 因为只会返回1MB以上的内存，所以需要加上1MB

.mem_get_ok:
    mov [total_mem_bytes], edx

; 进入保护模式

    ; 1. 打开A20
    in al, 0x92
    or al, 0000_0010b
    out 0x92, al

    ; 2. 加载GDT
    lgdt [gdt_ptr]

    ; 3. 将cr0的pe位置1
    mov eax, cr0
    or eax, 0x000000001
    mov cr0, eax

    jmp dword SELECTOR_CODE:p_mode_start     ; 刷新流水线

.error_hlt:
    hlt

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    mov byte [gs:160], 'P'
    mov byte [gs:161], 0x0c
    mov byte [gs:162], ' '
    mov byte [gs:163], 0x0c
    mov byte [gs:164], 'm'
    mov byte [gs:165], 0x0c
    mov byte [gs:166], 'o'
    mov byte [gs:167], 0x0c
    mov byte [gs:168], 'd'
    mov byte [gs:169], 0x0c
    mov byte [gs:170], 'e'
    mov byte [gs:171], 0x0c
    jmp $