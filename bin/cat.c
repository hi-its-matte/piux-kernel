#include <stdint.h>
#include "../kernel/vfs.h"

typedef void (*vga_puts_t)(const char*);
typedef void (*vga_putc_t)(char);

void cmd_cat(const char *filename, vga_puts_t vga_puts, vga_putc_t vga_putc) {
    if (*filename == '\0') {
        vga_puts("Usage: cat <filename>\n");
        return;
    }
    
    char buffer[8192];
    int fd = vfs_open(filename, VFS_O_READ);
    int bytes = fd < 0 ? -1 : vfs_read(fd, buffer, 8192);
    if (fd >= 0) vfs_close(fd);
    
    if (bytes < 0) {
        vga_puts("File not found\n");
        return;
    }
    
    for (int i = 0; i < bytes; i++) {
        vga_putc(buffer[i]);
    }
    if (bytes > 0) vga_putc('\n');
}
