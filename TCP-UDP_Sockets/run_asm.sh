nasm -f elf64 Logger.asm
ld Logger.o
strace ./a.out