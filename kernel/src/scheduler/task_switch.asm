section .text

global task_switch
global thread_init_trampoline

extern spinlock_release_no_sti

thread_init_trampoline:
    mov rdi, rbx
    mov rsi, rbp
    mov rdx, r12
    mov rcx, r13
    mov r8,  r14
    mov r9,  r15
    ret


task_switch:
    cli

    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov [rsi], rsp
    mov rsp, rdx
    call spinlock_release_no_sti
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    
    ret