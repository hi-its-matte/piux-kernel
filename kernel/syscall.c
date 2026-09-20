#include <stdint.h>
#include "syscall.h"
#include "vfs.h"
#include "process.h"
#include "memory.h"

static int syscall_from_user(const syscall_registers_t *registers) {
    uint32_t *return_frame = (uint32_t *)(uintptr_t)registers->original_esp;
    return (return_frame[1] & 3) == 3;
}

static int user_string_valid(const char *text) {
    if (!memory_range_valid(text, 1, 1)) return 0;
    for (uint32_t index = 0; index < 4096; index++) {
        if (!memory_range_valid(text + index, 1, 1)) return 0;
        if (text[index] == '\0') return 1;
    }
    return 0;
}

static int syscall_open(const char *path, int flags) {
    if (!path) return -1;
    return vfs_open(path, flags);
}

void syscall_dispatch(syscall_registers_t *registers) {
    int result = -1;
    int from_user = syscall_from_user(registers);
    switch (registers->eax) {
        case SYS_READ:
            if (from_user && !memory_range_valid((void *)registers->ecx, registers->edx, 1)) break;
            result = vfs_read((int)registers->ebx, (void *)registers->ecx, registers->edx);
            break;
        case SYS_WRITE:
            if (from_user && !memory_range_valid((void *)registers->ecx, registers->edx, 1)) break;
            result = vfs_write((int)registers->ebx, (const void *)registers->ecx, registers->edx);
            break;
        case SYS_OPEN:
            if (from_user && !user_string_valid((const char *)registers->ebx)) break;
            result = syscall_open((const char *)registers->ebx, (int)registers->ecx);
            break;
        case SYS_CLOSE:
            result = vfs_close((int)registers->ebx);
            break;
        case SYS_SEEK:
            result = vfs_seek((int)registers->ebx, registers->ecx);
            break;
        case SYS_GETPID:
            result = process_getpid();
            break;
        case SYS_GETUID:
            result = process_getuid();
            break;
        case SYS_YIELD:
            process_yield();
            result = 0;
            break;
        case SYS_EXIT:
            if (from_user) process_exit_user((int)registers->ebx);
            process_exit((int)registers->ebx);
            result = 0;
            break;
        default:
            break;
    }
    registers->eax = (uint32_t)result;
}

void syscall_init(void);