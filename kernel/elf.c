#include <stdint.h>
#include "elf.h"
#include "vfs.h"
#include "memory.h"

#define ELF_MAX_FILE_SIZE 8192
#define ELF_PT_LOAD 1
#define ELF_CLASS_32 1
#define ELF_TYPE_EXEC 2

typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf32_header_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed)) elf32_program_header_t;

static uint8_t file_buffer[ELF_MAX_FILE_SIZE];

static int read_whole_file(const char *path) {
    int fd = vfs_open(path, VFS_O_READ);
    int total = 0;
    if (fd < 0) return -1;
    for (;;) {
        int chunk = vfs_read(fd, file_buffer + total, sizeof(file_buffer) - (uint32_t)total);
        if (chunk <= 0) break;
        total += chunk;
        if ((uint32_t)total >= sizeof(file_buffer)) break;
    }
    vfs_close(fd);
    return total;
}

int elf_load(const char *path, uint32_t *entry_point) {
    int total = read_whole_file(path);
    elf32_header_t *header;
    if (total < (int)sizeof(elf32_header_t)) return -1;

    header = (elf32_header_t *)file_buffer;
    if (header->e_ident[0] != 0x7f || header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' || header->e_ident[3] != 'F') return -1;
    if (header->e_ident[4] != ELF_CLASS_32 || header->e_type != ELF_TYPE_EXEC) return -1;
    if ((uint32_t)header->e_phoff + (uint32_t)header->e_phnum * header->e_phentsize > (uint32_t)total) return -1;

    for (uint16_t index = 0; index < header->e_phnum; index++) {
        elf32_program_header_t *program = (elf32_program_header_t *)
            (file_buffer + header->e_phoff + index * header->e_phentsize);
        uint8_t *destination;
        uint8_t *source;
        if (program->p_type != ELF_PT_LOAD) continue;
        if (!memory_range_valid((void *)(uintptr_t)program->p_vaddr, program->p_memsz, 1)) return -1;
        if (program->p_offset + program->p_filesz > (uint32_t)total) return -1;
        destination = (uint8_t *)(uintptr_t)program->p_vaddr;
        source = file_buffer + program->p_offset;
        for (uint32_t byte = 0; byte < program->p_filesz; byte++) destination[byte] = source[byte];
        for (uint32_t byte = program->p_filesz; byte < program->p_memsz; byte++) destination[byte] = 0;
    }

    *entry_point = header->e_entry;
    return 0;
}
