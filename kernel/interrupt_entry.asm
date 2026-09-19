BITS 32

section .text
extern timer_tick
extern outb
global timer_entry
global page_fault_entry
global general_protection_entry

timer_entry:
    pusha
    call timer_tick
    mov al, 0x20
    mov dx, 0x20
    out dx, al
    popa
    iretd

page_fault_entry:
general_protection_entry:
    cli
.halt:
    hlt
    jmp .halt