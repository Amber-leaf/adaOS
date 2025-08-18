bits 32
global setPaging
global setCompatibility
global checkCPUID
global queryLongMode

; ---------- constants ----------
EFLAGS_ID          equ (1 << 21)
CPUID_EXTENSIONS   equ 0x80000000
CPUID_EXT_FEATURES equ 0x80000001

; page table flags (common low bits)
P_PRESENT  equ 1
P_RW       equ (1 << 1)

CR4_PAE    equ (1 << 5)
CR0_PE     equ (1 << 0)
CR0_PG     equ (1 << 31)

EFER_MSR   equ 0xC0000080
EFER_LME   equ (1 << 8)

ENTRIES_PER_PT equ 512
PAGE_SIZE      equ 4096

; ---------- page tables in low memory ----------
section .bss align=4096
pml4:   resb 4096
pdp:    resb 4096
pd:     resb 4096
pt0:    resb 4096

section .text

checkCPUID:
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

queryLongMode:
    mov eax, CPUID_EXTENSIONS
    cpuid
    cmp eax, CPUID_EXT_FEATURES
    setae al
    movzx eax, al
    ret

; -----------------------------------------------
; Build a minimal 4-level paging structure:
;   PML4[0] -> PDP[0] -> PD[0] -> PT0 (4K pages)
;   PT0 maps 0..2MiB identity with RW,P
; -----------------------------------------------
setPaging:
    ; clear all four tables
    xor eax, eax
    mov edi, pml4
    mov ecx, PAGE_SIZE/4
    rep stosd

    mov edi, pdp
    mov ecx, PAGE_SIZE/4
    rep stosd

    mov edi, pd
    mov ecx, PAGE_SIZE/4
    rep stosd

    mov edi, pt0
    mov ecx, PAGE_SIZE/4
    rep stosd

    ; link PML4[0] -> PDP
    mov eax, pdp           ; low phys addr
    or  eax, P_PRESENT | P_RW
    mov dword [pml4 + 0], eax
    mov dword [pml4 + 4], 0

    ; link PDP[0] -> PD
    mov eax, pd
    or  eax, P_PRESENT | P_RW
    mov dword [pdp + 0], eax
    mov dword [pdp + 4], 0

    ; link PD[0] -> PT0 (4K pages, not 2MiB PS)
    mov eax, pt0
    or  eax, P_PRESENT | P_RW
    mov dword [pd + 0], eax
    mov dword [pd + 4], 0

    ; fill PT0: identity map first 2MiB (512 * 4KiB)
    xor ebx, ebx                    ; physical base = 0
    mov edi, pt0
    mov ecx, ENTRIES_PER_PT
.fill_pt0:
    mov eax, ebx
    or  eax, P_PRESENT | P_RW
    mov dword [edi], eax            ; low dword (addr + flags)
    mov dword [edi+4], 0            ; high dword (addr[63:32]) = 0 for <4GiB
    add ebx, PAGE_SIZE
    add edi, 8
    loop .fill_pt0

    ; load CR3 with PHYSICAL address of PML4
    mov eax, pml4
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or  eax, CR4_PAE
    mov cr4, eax

    ret

; Turn on long mode (LME) and paging.
; Note: to actually execute 64-bit code you still need a 64-bit GDT and a far jump.
setCompatibility:
    ; enable LME
    mov ecx, EFER_MSR
    rdmsr
    or eax, EFER_LME
    wrmsr

    ; enable PE and PG (PE can be set earlier if you like)
    mov eax, cr0
    or  eax, CR0_PE | CR0_PG
    mov cr0, eax
    ret
