   .section .note.GNU-stack,"",@progbits
.text

nr_syscall = 10

.global handler_syscall
handler_syscall:
    cmpl $nr_syscall-1, %eax
    ja bad_sys_call

    pushl %edx       // Sauvegarde des registres
    pushl %ecx
    pushl %ebx

    call *syscall_table(,%eax,4)

    popl %ebx        // Restauration (ordre inverse)
    popl %ecx
    popl %edx
    iret

bad_sys_call:
    movl $-1, %eax
    iret