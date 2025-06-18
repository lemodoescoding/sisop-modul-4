SRC = src
BIN = bin

NASM = nasm
BCC = bcc
LD86 = ld86
DD = dd

prepare:
	mkdir -p $(BIN)
	$(DD) if=/dev/zero of=$(BIN)/floppy.img bs=512 count=2880 conv=notrunc

bootloader:
	$(NASM) -f bin $(SRC)/bootloader.asm -o $(BIN)/bootloader.bin

stdlib:
	$(BCC) -Iinclude -ansi -c $(SRC)/std_lib.c -o $(BIN)/std_lib.o

kernel:
	$(BCC) -Iinclude -ansi -c $(SRC)/kernel.c -o $(BIN)/kernel.o
	$(NASM) -f as86 $(SRC)/kernel.asm -o $(BIN)/kernel_asm.o

link:
	$(LD86) -o $(BIN)/kernel.bin -d $(BIN)/kernel.o $(BIN)/kernel_asm.o $(BIN)/std_lib.o
	$(DD) if=$(BIN)/bootloader.bin of=$(BIN)/floppy.img bs=512 conv=notrunc
	$(DD) if=$(BIN)/kernel.bin of=$(BIN)/floppy.img bs=512 seek=1 conv=notrunc

build: prepare bootloader stdlib kernel link

run:
	bochs -f bochsrc.txt
