/* Minimal freestanding test program for the Piux `run` command.
   No libc: syscalls are invoked directly via int 0x80. */

static inline int piux_syscall(int number, int a, int b, int c) {
    int result;
    asm volatile ("int $0x80"
                  : "=a"(result)
                  : "a"(number), "b"(a), "c"(b), "d"(c)
                  : "memory");
    return result;
}

void _start(void) {
    const char *message = "Hello from user mode!\n";
    int length = 0;
    while (message[length]) length++;
    piux_syscall(1 /* SYS_WRITE */, 1 /* stdout */, (int)(long)message, length);
    piux_syscall(8 /* SYS_EXIT */, 42, 0, 0);
    for (;;) {
    }
}
