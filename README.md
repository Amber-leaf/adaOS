# adaOS
A small in development kernel.

## Building:
`make run` will start QEMU and run the iso.

on Arch Linux:
``` shell
git clone https://github.com/Amber-leaf/adaOS/
cd ./adaOS/kernel/
./get-deps
cd ../
make
```
on Debian / Ubuntu
``` shell
git clone https://github.com/Amber-leaf/adaOS/
cd ./adaOS/kernel/
./get-deps-apt
cd ../
make
```

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
Basic ACPI parsing. (todo: ACPICA or uACPI support, requires threading and synchronization primitives or lots of work, respectivly)  
Configuration of IO APICs.  
Basic serial debug shell.  
