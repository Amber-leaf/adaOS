bits 32

global check_CPUID
global check_long_mode

EFLAGS_ID          equ (1 << 21)
CPUID_EXTENSIONS   equ 0x80000000
CPUID_EXT_FEATURES equ 0x80000001

section .text

check_CPUID:
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

check_long_mode:
    mov eax, CPUID_EXTENSIONS
    cpuid
    cmp eax, CPUID_EXT_FEATURES
    setae al
    movzx eax, al
    ret