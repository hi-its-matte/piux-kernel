#include <stdint.h>
#include "../kernel/rtl8139.h"
#include "../kernel/net.h"
#include "../kernel/vfs.h"
#include "../kernel/ext2.h"
#include "../kernel/http.h"

typedef void (*vga_puts_t)(const char*);
typedef void (*vga_putc_t)(char);

/* Points at a plain HTTP mirror of https://github.com/projectpiux/pix-pkgmanager,
   serving <pkg>/<pkg>.elf and <pkg>/info under /pkgs. Real github.com cannot be
   used here: it requires DNS and HTTPS, neither of which Piux implements yet. */
static uint32_t repo_ip;
static uint16_t repo_port;
static int repo_configured;

static int split_tokens(const char *input, char tokens[][64], int max_tokens) {
    int count = 0;
    int index = 0;
    while (input[index] && count < max_tokens) {
        int length = 0;
        while (input[index] == ' ') index++;
        if (!input[index]) break;
        while (input[index] && input[index] != ' ' && length < 63) tokens[count][length++] = input[index++];
        tokens[count][length] = '\0';
        count++;
    }
    return count;
}

static int match(const char *text, const char *word) {
    int index = 0;
    while (text[index] && word[index] && text[index] == word[index]) index++;
    return text[index] == '\0' && word[index] == '\0';
}

static void append_str(char *dest, int *length, const char *src) {
    for (int index = 0; src[index]; index++) dest[(*length)++] = src[index];
    dest[*length] = '\0';
}

static void build_pkg_filename(char *out, const char *pkg) {
    int length = 0;
    append_str(out, &length, pkg);
    append_str(out, &length, ".elf");
}

static void build_remote_path(char *out, const char *pkg, const char *filename) {
    int length = 0;
    append_str(out, &length, "/projectpiux/pix-pkgmanager/main/pkgs/");
    append_str(out, &length, pkg);
    append_str(out, &length, "/");
    append_str(out, &length, filename);
}

static void build_local_dir(char *out, const char *pkg) {
    int length = 0;
    append_str(out, &length, "/pkgs/");
    append_str(out, &length, pkg);
}

static void build_local_path(char *out, const char *pkg, const char *filename) {
    int length = 0;
    append_str(out, &length, "/pkgs/");
    append_str(out, &length, pkg);
    append_str(out, &length, "/");
    append_str(out, &length, filename);
}

static void show_usage(vga_puts_t vga_puts) {
    vga_puts("Usage: pix [config <ip> <port> | install <pkg> | remove <pkg> | info <pkg> | where <pkg>]\n");
}

static int fetch_package_file(const char *pkg, const char *filename, const char *local_path) {
    char remote_path[128];
    char host[16];
    int fd;
    int bytes;

    build_remote_path(remote_path, pkg, filename);
    net_format_ip(repo_ip, host);

    fd = vfs_open(local_path, VFS_O_WRITE | VFS_O_CREATE);
    if (fd < 0) return -1;
    bytes = http_get_to_fd(repo_ip, repo_port, remote_path, host, fd);
    vfs_close(fd);
    return bytes;
}

void cmd_pix(const char *param, vga_puts_t vga_puts, vga_putc_t vga_putc) {
    char tokens[3][64];
    int count;

    if (param[0] == '\0') {
        show_usage(vga_puts);
        return;
    }

    count = split_tokens(param, tokens, 3);
    if (count == 0) {
        show_usage(vga_puts);
        return;
    }

    if (match(tokens[0], "config") && count == 3) {
        uint16_t port = 0;
        for (int index = 0; tokens[2][index]; index++) port = (uint16_t)(port * 10 + (tokens[2][index] - '0'));
        repo_ip = net_parse_ip(tokens[1]);
        repo_port = port;
        repo_configured = 1;
        vga_puts("Pix repository configured\n");
        return;
    }

    if (match(tokens[0], "info") && count == 2) {
        char local_path[80];
        char buffer[512];
        int bytes;
        build_local_path(local_path, tokens[1], "info");
        bytes = ext2_read_file_by_path(local_path, buffer, sizeof(buffer) - 1);
        if (bytes < 0) {
            vga_puts("Package '");
            vga_puts(tokens[1]);
            vga_puts("' is not installed\n");
            return;
        }
        buffer[bytes] = '\0';
        vga_puts(buffer);
        vga_putc('\n');
        return;
    }

    if (match(tokens[0], "where") && count == 2) {
        char local_dir[80];
        build_local_dir(local_dir, tokens[1]);
        if (ext2_find_inode_by_path(local_dir) < 0) {
            vga_puts("Package '");
            vga_puts(tokens[1]);
            vga_puts("' is not installed\n");
            return;
        }
        vga_puts(local_dir);
        vga_putc('\n');
        return;
    }

    if (match(tokens[0], "remove") && count == 2) {
        char elf_filename[68];
        char elf_local[80];
        char info_local[80];
        int removed_any = 0;

        build_pkg_filename(elf_filename, tokens[1]);
        build_local_path(elf_local, tokens[1], elf_filename);
        build_local_path(info_local, tokens[1], "info");

        if (ext2_delete_file_by_path(elf_local) == 0) removed_any = 1;
        if (ext2_delete_file_by_path(info_local) == 0) removed_any = 1;

        if (!removed_any) {
            vga_puts("Package '");
            vga_puts(tokens[1]);
            vga_puts("' is not installed\n");
            return;
        }
        vga_puts("Removed '");
        vga_puts(tokens[1]);
        vga_puts("'\n");
        return;
    }

    if (match(tokens[0], "install") && count == 2) {
        char local_dir[80];
        char elf_filename[68];
        char elf_local[80];
        char info_local[80];
        int elf_bytes;
        int info_bytes;

        if (!repo_configured) {
            vga_puts("Pix repository not configured. Use: pix config <ip> <port>\n");
            vga_puts("(this must be a plain HTTP mirror of the repo layout; real github.com needs DNS+HTTPS, not supported yet)\n");
            return;
        }
        if (!rtl8139_is_ready()) {
            vga_puts("No network interface detected\n");
            return;
        }

        if (ext2_find_inode_by_path("/pkgs") < 0) ext2_create_directory_by_path("/pkgs");
        build_local_dir(local_dir, tokens[1]);
        if (ext2_find_inode_by_path(local_dir) < 0 && ext2_create_directory_by_path(local_dir) < 0) {
            vga_puts("Failed to create package directory\n");
            return;
        }

        build_pkg_filename(elf_filename, tokens[1]);
        build_local_path(elf_local, tokens[1], elf_filename);
        build_local_path(info_local, tokens[1], "info");

        elf_bytes = fetch_package_file(tokens[1], elf_filename, elf_local);
        info_bytes = fetch_package_file(tokens[1], "info", info_local);

        if (elf_bytes <= 0 || info_bytes <= 0) {
            vga_puts("Package '");
            vga_puts(tokens[1]);
            vga_puts("' not found or download failed\n");
            return;
        }

        vga_puts("Installed '");
        vga_puts(tokens[1]);
        vga_puts("' to ");
        vga_puts(local_dir);
        vga_putc('\n');
        return;
    }

    show_usage(vga_puts);
}
