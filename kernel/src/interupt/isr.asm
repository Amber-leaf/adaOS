section .text

extern exception_handler
global isr_stub_table

global isr_0x20
global isr_0x24
global isr_0x70
global isr_0xf0
global isr_0xf1
global isr_0xf2
global isr_0xf3
global isr_0xf4
global isr_0xf5
global isr_0xf6

%macro no_err 1
global isr%1
isr%1:
    push 0
    push %1
    jmp exception_handler_asm
%endmacro

%macro err 1
global isr%1
isr%1:
    push %1
    jmp exception_handler_asm
%endmacro

exception_handler_asm:
    cli
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call exception_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    sti
    iretq

isr_stub_table:
%assign i 0 
%rep    32
    dq isr%+i
%assign i i+1 
%endrep

no_err 0
no_err 1
no_err 2
no_err 3
no_err 4
no_err 5
no_err 6
no_err 7
err    8
no_err 9
err    10
err    11
err    12
err    13
err    14
no_err 15
no_err 16
err    17
no_err 18
no_err 19
no_err 20
no_err 21
no_err 22
no_err 23
no_err 24
no_err 25
no_err 26
no_err 27
no_err 28
no_err 29
err    30
no_err 31

isr_0x20:
no_err 0x20 ; pit

isr_0x24:
no_err 0x24 ; serial

isr_0x70:
no_err 0x70 ; syscall

isr_0xf0:
no_err 0xf0 ; apic spurious vector handler

isr_0xf1:
no_err 0xf1 ; apic timer handler

isr_0xf2:
no_err 0xf2 ; apic thermal handler

isr_0xf3:
no_err 0xf3 ; apic performance counter handler

isr_0xf4:
no_err 0xf4 ; apic lint0 handler

isr_0xf5:
no_err 0xf5 ; apic lint1 handler

isr_0xf6:
err 0xf6    ; apic error handler





