
    .globl handler_irq50
handler_irq50:
    pushl %eax
    pushl %ebx
    pushl %ecx

    call handler_IT50_C

    popl %ecx
    popl %ebx
    popl %eax
    iret