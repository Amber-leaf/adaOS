bits 32

global check_cpuid

EFLAGS_ID          equ (1 << 21)

section .text

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, EFLAGS_ID
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    setnz al
    movzx eax, al
    ret