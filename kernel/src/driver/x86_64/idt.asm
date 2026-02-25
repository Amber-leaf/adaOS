section .text

global lidt

lidt: ; load the IDT into the register.
    lidt [rdi]
    sti
    ret