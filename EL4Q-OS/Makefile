all: el4q.iso

kernel.bin:
	nasm -f elf32 kernel/src/multiboot.asm -o boot.o
	gcc -m32 -c kernel/src/kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
	ld -m elf_i386 -T kernel/src/linker.ld -o kernel.bin boot.o kernel.o

el4q.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	grub-mkrescue -o el4q.iso iso

run: el4q.iso
	qemu-system-i386 -cdrom el4q.iso

clean:
	rm -rf *.o kernel.bin el4q.iso iso/boot/kernel.bin
