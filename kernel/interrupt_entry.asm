BITS 32

section .text
extern timer_tick
extern outb
extern mouse_irq_handler
global timer_entry
global mouse_entry
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

mouse_entry:
    pusha
    call mouse_irq_handler
    mov al, 0x20
    mov dx, 0xa0
    out dx, al
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