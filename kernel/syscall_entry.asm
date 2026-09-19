BITS 32

section .text
extern syscall_dispatch
global syscall_entry

syscall_entry:
    pusha
    push esp
    call syscall_dispatch
    add esp, 4
    popa
    iretd