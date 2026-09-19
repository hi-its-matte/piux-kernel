#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

enum {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_OPEN = 2,
    SYS_CLOSE = 3,
    SYS_SEEK = 4,
    SYS_GETPID = 5,
    SYS_GETUID = 6,
    SYS_YIELD = 7,
    SYS_EXIT = 8
};

typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t original_esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
} syscall_registers_t;

void syscall_init(void);
void syscall_dispatch(syscall_registers_t *registers);

static inline int syscall_invoke(uint32_t number, uint32_t argument_one,
                                 uint32_t argument_two, uint32_t argument_three) {
    int result;
    asm volatile ("int $0x80"
                  : "=a"(result)
                  : "a"(number), "b"(argument_one), "c"(argument_two), "d"(argument_three)
                  : "memory");
    return result;
}

#endif