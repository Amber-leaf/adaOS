section .text

global lgdt
global ltr
global reload_segments

lgdt: ; load the GDT into the register.
    lgdt [rdi]
    ret

ltr: ; load the task register to say we support multitasking and that out TSS is at di in the GDT.
    ltr di
    ret

reload_segments:
   push 0x08
   lea rax, [rel reload_CS]
   push rax
   retfq

reload_CS:
   mov ax, 0x10
   mov ds, ax
   mov es, ax
   mov fs, ax
   mov gs, ax
   mov ss, ax
   ret
