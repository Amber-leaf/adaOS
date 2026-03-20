# adaOS
A small in development kernel.

## Features:
Loads a GDT and IDT.  
Masks PIC interrupts.  
Functional rs232 driver.  
Uses PIT and APIC as a hardware timer.  
RTC functionality.  
Play sound on the PC speaker using the PIT.  
Physical memory manager (bitmap allocator).  
Virtual memory manager.  
Heap manager.  
APIC functionality.  
Basic ACPI parsing. (todo: ACPICA or uAPIC support, requires threading and synchronization primitives or lots of work, respectivly)  
Configuration of IO APICs.  
Basic serial debug shell.  
