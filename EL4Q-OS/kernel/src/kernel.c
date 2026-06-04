#include <stdint.h>
#include <stddef.h>

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void vga_putc(char c, int x, int y) {
    uint16_t *vga_buffer = (uint16_t *)0xB8000;
    vga_buffer[y * 80 + x] = (uint16_t)c | (uint16_t)0x0F00;
}

void vga_clear() {
    uint16_t *vga_buffer = (uint16_t *)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t)' ' | (uint16_t)0x0F00;
    }
}

void vga_print(char *str, int y) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putc(str[i], i, y);
    }
}

unsigned char keyboard_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void wait(int ticks) {
    for (volatile int i = 0; i < ticks * 1000000; i++);
}

void play_boot_animation() {
    vga_clear();
    vga_print("EL4Q-OS is loading...", 10);
    for (int i = 0; i < 20; i++) {
        vga_putc('#', 30 + i, 12);
        wait(5);
    }
    wait(10);
    vga_clear();
}

int strcmp(char *s1, char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void handle_command(char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        vga_print("Available: help, clear, version, reboot", 5);
    } else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
    } else if (strcmp(cmd, "version") == 0) {
        vga_print("EL4Q-OS v0.1", 5);
    } else if (strcmp(cmd, "reboot") == 0) {
        uint8_t good = 0x02;
        while (good & 0x02) good = inb(0x64);
        outb(0x64, 0xFE);
    } else {
        vga_print("Command not found nigg-.", 5);
    }
}

void read_line(char *buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                if (c == '\n') {
                    buffer[i] = '\0';
                    return;
                } else if (c == '\b' && i > 0) {
                    i--;
                    vga_putc(' ', i + 2, 2); // Offset by 2 for "> " ابب
                } else if (c != 0) {
                    buffer[i] = c;
                    vga_putc(c, i + 2, 2); // Offset by 2 for "> " ابب
                    i++;
                }
            }
        }
    }
    buffer[i] = '\0';
}

struct gdt_entry {
    uint16_t limit_low; uint16_t base_low; uint8_t base_middle;
    uint8_t access; uint8_t granularity; uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr { uint16_t limit; uint32_t base; } __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gp;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_install() {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (uint32_t)&gdt;
    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    asm volatile("lgdt %0" : : "m"(gp));
}

void kernel_main() {
    gdt_install();
    play_boot_animation();
    
    vga_print("EL4Q-OS Shell Ready.", 0);
    vga_print("> ", 2);

    char cmd_buffer[64];
    while (1) {
        read_line(cmd_buffer, 64);
        handle_command(cmd_buffer);
        
        for(int x=0; x<80; x++) vga_putc(' ', x, 2);
        vga_print("> ", 2);
    }
}
