global _start

section .data
    fdf dw  0, 0, 0, 0
    sdf dw  0, 0, 0, 0
    l   dw  0, 0, 0, 0
    p   db  `log.txt\0`
    m   db  'Hello'
    s1  dw  1 ;
    s   dw  2 ;AF_INET
        db  13h,8Fh ;=5007
    s2  db  0E2h,01h,01h,01h ;226.1.1.1
        db  0,0,0,0,0,0,0,0
    nl  db  0x0D, 0xA
    spc db  0x20
    tim dw  0, 0, 0, 0
section .bss
	buf:    resb    129

section .text

_start:

mov rax, 2
mov rdi, p
mov rsi, 0x441   ; O_CREAT| O _WRONLY | O_APPEND
syscall

mov [fdf],rax  ; file file descriptor

mov rax,41 ;socket()
mov rdi,2 ;AF_INET
mov rsi,2 ;SOCK_DGRAM
mov rdx,0 ;flags
syscall

mov [sdf],rax ;socket file descriptor

mov rdi,[sdf]
mov rax,54 ;setsockopt()
mov rsi,1 ;SOL_SOCKET
mov rdx,2 ;SO_REUSEADDR
mov r10,s1 
mov r8,4 
syscall


mov rdi,[sdf]
mov rax,49 ;bind()
mov rsi,s
mov rdx,16
syscall


mov rdi,[sdf]
mov rax,54 ;setsockopt()
mov rsi,0 ;IPPROTO_IP
mov rdx,35 ;IP_ADD_MEMBERSHIP
mov r10,s2 
mov r8,8 
syscall

read_loop:

    mov rdi,[sdf]
    mov rax,0 ;read()
    mov rsi,buf
    mov rdx,128
    syscall

    mov [l],rax

    mov rdi,tim
    mov rax,201 ;time()
    syscall

    mov rdi,3
    mov rax,1 ;write()
    mov rsi,spc
    mov rdx,1
    syscall

    mov rdi,3
    mov rax,1 ;write()
    mov rsi,buf
    mov rdx,[l]
    syscall

    mov rdi,3
    mov rax,1 ;write()
    mov rsi,nl
    mov rdx,2
    syscall

    jmp read_loop
