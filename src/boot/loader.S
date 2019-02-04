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

VIDEO_DESC:      dd 0x80000007               ; limit = (0xbffff-0xb8000)/4k = 0x7
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
    mov eax, 0x0000e820                ; 功能号，每次调用结束后都会变成'SMAP'
    mov ecx, 20                        ; 缓冲区字节大小，每次调用结束后为实际写入的字节数
    int 0x15
    jc .e820_failed_so_try_e801;       ; CF置1为调用失败
    add di, cx                         ; 更新es:di
    inc word [ards_nr]
    cmp ebx, 0                         ; CF不为1且ebx为0,表示结束
    jnz .e820_mem_get_loop

    ; 获取数组中 base_add_low + length_low 最大的元素，即内存的容量
    mov cx, [ards_nr]
    mov ebx, ards_buf
    xor edx, edx                       ; edx保存内存的容量
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

    ; mov byte [gs:160], 'P'                   ; 第二行第一个字符
    ; mov byte [gs:161], 0x0c

; 加载kernel

    mov eax, KERNEL_START_SECTOR             ; 起始扇区号
    mov ebx, KERNEL_BIN_BASE_ADDR            ; 内存起始地址
    mov ecx, 200                             ; 扇区数

    call rd_disk_m_32

; 进入分页

    call setup_page                          ; 创建页目录项和页表

    sgdt [gdt_ptr]                           ; 存储原来的gdt

    ; 修改显存的段基址到3GB以上，即给段基址加上0xc0000000
    mov ebx, [gdt_ptr + 2]
    or dword [ebx + 0x18 + 4], 0xc0000000

    add dword [gdt_ptr + 2], 0xc0000000      ; gdt的基址也加上0xc0000000，映射到3GB以上的地址空间

    add esp, 0xc0000000                      ; 栈指针也映射到内核空间

    ; 将页目录地址给cr3
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    ; 打开cr0的pg位
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    lgdt [gdt_ptr]                           ; 重新加载gdt

    ; mov byte [gs:320], 'V'                   ; 第三行第一个字符
    ; mov byte [gs:321], 0x0c

    jmp SELECTOR_CODE:enter_kernel           ; 强制刷新流水线，更新gdt

enter_kernel:
    call kernel_init
    mov esp, 0xc009f000

    jmp KERNEL_ENTRY_POINT



; 功能：从硬盘读取数据到内存
; 参数：
;   eax: 起始扇区号
;   ebx: 内存起始地址
;   ecx: 扇区数量
rd_disk_m_32:
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
    mov [ebx], ax
    add ebx, 2
    loop .go_on_read
    ret

; 将kernel.bin中的segment拷贝到编译的地址
kernel_init:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    mov dx, [KERNEL_BIN_BASE_ADDR + 42]      ; 得到e_phentsize，即program header的大小
    mov ebx, [KERNEL_BIN_BASE_ADDR + 28]     ; 得到e_phoff，即第一个program header在文件中的偏移量

    add ebx, KERNEL_BIN_BASE_ADDR
    mov cx, [KERNEL_BIN_BASE_ADDR + 44]      ; 得到e_phnum，即program header的数量

.each_segment:
    cmp byte [ebx + 0], PT_NULL
    je .PTNULL

    push dword [ebx + 16]                    ; 压入p_filesz，即program的字节数
    mov eax, [ebx + 4]                       ; eax存储p_offset，即program的偏移量
    add eax, KERNEL_BIN_BASE_ADDR

    push eax
    push dword [ebx + 8]                     ; 压入p_vaddr，即program起始的虚拟地址
    call mem_cpy
    add esp, 12
.PTNULL:
    add ebx, edx                             ; 指向下一个program header entry

    loop .each_segment
    ret

; 功能：逐字节拷贝
; 输入：栈中三个参数
mem_cpy:
    cld                                      ; 设置movsb指令对edi和esi的更新方式，为增加1B
    push ebp
    mov ebp, esp
    push ecx
    mov edi, [ebp + 8]                       ; dst
    mov esi, [ebp + 12]                      ; src
    mov ecx, [ebp + 16]                      ; size
    rep movsb                                ;逐字节拷贝

    ; 恢复环境
    pop ecx
    pop ebp
    ret

; 功能：初始化页目录项和页表
setup_page:
    ; 将页目录项都清零
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

    ; 
.create_pde:
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000                          ; 指向第一个页表;
    mov ebx, eax
    or eax, PG_US_U | PG_RW_W | PG_P         ; 设置属性
    mov [PAGE_DIR_TABLE_POS + 0x0], eax
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax    ; 第768个页目录项，为第3GB地址的起始
    sub eax, 0x1000                          ; 指向页第一个目录项
    mov [PAGE_DIR_TABLE_POS + 1023*4], eax   ; 将最后一个页目录项指向第一个页目录项

    ; 创建页表项PTE
    mov ecx, 256                             ; 0～1M地址的页表项
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
    mov [ebx + esi*4], edx                   ; ebx为第一个页表的地址
    add edx, 0x1000
    inc esi
    loop .create_pte

    ; 创建内核其他页表的PDE
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000                          ; 指向第二个页表
    or eax, PG_US_U | PG_RW_W | PG_P
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 254
    mov esi, 769                             ; 初始化第769～1022的所有页目录项。
.create_kernel_pde:
    mov [ebx + esi*4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernel_pde
    ret

