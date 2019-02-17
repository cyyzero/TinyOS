BIN_DIR = ./bin
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I ./src/lib/ -I ./src/lib/kernel/ -I ./src/lib/user -I ./src/device -I ./src/kernel
ASFLAGS = -f elf
CFLAGS = -Wall $(LIB) -c -m32 -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BIN_DIR)/kernel.map
OBJS = $(BIN_DIR)/main.o $(BIN_DIR)/init.o $(BIN_DIR)/interrupt.o $(BIN_DIR)/timer.o \
	$(BIN_DIR)/kernel.o $(BIN_DIR)/print1.o $(BIN_DIR)/print2.o $(BIN_DIR)/debug.o  \
	$(BIN_DIR)/string.o $(BIN_DIR)/bitmap.o $(BIN_DIR)/memory.o

# C代码编译
$(BIN_DIR)/main.o: ./src/kernel/main.c ./src/lib/kernel/print.h ./src/lib/stdint.h ./src/kernel/init.h ./src/kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/init.o: ./src/kernel/init.c ./src/kernel/init.h ./src/lib/kernel/print.h ./src/lib/stdint.h ./src/kernel/interrupt.h ./src/device/timer.h ./src/kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/interrupt.o: ./src/kernel/interrupt.c ./src/kernel/interrupt.h ./src/lib/stdint.h ./src/kernel/global.h ./src/lib/kernel/io.h ./src/lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/timer.o: ./src/device/timer.c ./src/device/timer.h ./src/lib/stdint.h ./src/lib/kernel/io.h ./src/lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/debug.o: ./src/kernel/debug.c ./src/kernel/debug.h ./src/lib/kernel/print.h ./src/lib/stdint.h ./src/kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/print1.o: ./src/lib/kernel/print.c ./src/lib/kernel/print.h ./src/lib/stdint.h
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/string.o: ./src/lib/string.c ./src/lib/string.h ./src/kernel/global.h ./src/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/bitmap.o: ./src/lib/kernel/bitmap.c ./src/lib/kernel/bitmap.h ./src/lib/stdint.h ./src/lib/string.h ./src/lib/kernel/print.h ./src/kernel/interrupt.h ./src/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/memory.o: ./src/kernel/memory.c ./src/kernel/memory.h ./src/lib/stdint.h ./src/lib/kernel/print.h ./src/lib/kernel/bitmap.h ./src/kernel/global.h ./src/kernel/debug.h ./src/lib/string.h
	$(CC) $(CFLAGS) $< -o $@

# 编译汇编代码
$(BIN_DIR)/kernel.o: ./src/kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BIN_DIR)/print2.o: ./src/lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

# 链接所有目标
$(BIN_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: mk_dir hd clean all

mk_dir:
	if [[! -d $(BIN_DIR)]]; then mkdir $(BIN_DIR); fi

hd:
	dd if=$(BIN_DIR)/kernel.bin of=./hd60M.img bs=512 count=200 seek=9 conv=notrunc

clean:
	cd $(BIN_DIR) && rm -f ./*

build: $(BIN_DIR)/kernel.bin

all: mk_dir build hd