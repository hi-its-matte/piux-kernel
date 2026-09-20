BITS 32

section .text
global process_context_save
global process_context_restore
global process_enter_user_mode

; int process_context_save(process_context_t *context)
; Returns 0 on the initial call, or the value passed to
; process_context_restore when resumed later.
process_context_save:
    mov eax, [esp+4]
    mov ecx, [esp]
    mov [eax], ecx
    lea edx, [esp+4]
    mov [eax+4], edx
    mov [eax+8], ebp
    mov [eax+12], ebx
    mov [eax+16], esi
    mov [eax+20], edi
    xor eax, eax
    ret

; void process_context_restore(process_context_t *context, int value)
process_context_restore:
    mov eax, [esp+4]
    mov edx, [esp+8]
    mov ebp, [eax+8]
    mov ebx, [eax+12]
    mov esi, [eax+16]
    mov edi, [eax+20]
    mov ecx, [eax]
    mov esp, [eax+4]
    mov eax, edx
    jmp ecx

; void process_enter_user_mode(uint32_t entry, uint32_t user_stack)
process_enter_user_mode:
    mov eax, [esp+4]
    mov ecx, [esp+8]
    mov dx, 0x23
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx
    push dword 0x23
    push ecx
    pushfd
    pop edx
    or edx, 0x200
    push edx
    push dword 0x1b
    push eax
    iretd
