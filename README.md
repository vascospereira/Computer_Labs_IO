# Computer_Labs_IO

Program at the HW interface level of the most common PC I/O Devices and develop system-level programs. Developed in MINIX 3 which is a free, open-source, operating system designed to be highly reliable, flexible, and secure. It is based on a tiny microkernel running in kernel mode with the rest of the operating system running as a number of isolated, protected, processes in user mode. It runs on x86 and ARM CPUs, is compatible with NetBSD, and runs thousands of NetBSD packages. For testing/evaluation purposes was also developed a set of test functions.

# lab1
PC's video graphics adapter in text mode, by accessing directly to its memory. Write in the C programming language several functions for Minix to print characters on the screen, by directly modifying the video RAM (VRAM) area for text mode rather than using the I/O functions of the standard C library or of the POSIX standard.
# lab2
Write in the C programming language several functions to use the PC's timer.
Every PC "has" an i8254 an integrated circuit (IC) with 3 timers.
A timer has a 16 bit counter, two input lines, Clock and Enable, and an output line Out.
Each timer is programmed independently of the other timers. Programming a timer requires two steps:

-Specifying the timer operating mode, by writing a control word to the control register;

-Loading the counter initial value, by writing to the counter register.
