# adaOS
A small in development kernel.

## Features:
Loads a GDT and IDT.
Masks PIC interrupts.
Functional rs232 driver.
Sets up the PIT as a hardware timer.
Play sound on the PC speaker using the PIT.
Physical memory manager (bitmap allocator).
Virtual memory manager.
Heap manager.
APIC functionality.
Basic ACPI parsing. (todo: ACPICA support, requires threading and synchronization primitives)
Configuration of IO APICs.
Basic serial debug shell.