%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
mov byte [gs:0x00], 'L'
mov byte [gs:0x01], 0x0c

mov byte [gs:0x02], 'o'
mov byte [gs:0x03], 0x0c

mov byte [gs:0x04], 'a'
mov byte [gs:0x05], 0x0c

mov byte [gs:0x06], 'd'
mov byte [gs:0x07], 0x0c

mov byte [gs:0x08], 'e'
mov byte [gs:0x09], 0x0c

mov byte [gs:0x0a], 'r'
mov byte [gs:0x0b], 0x0c

jmp $