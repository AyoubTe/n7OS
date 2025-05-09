.globl timer_handler
timer_handler:
    pusha
    call timer_handler_c
    popa 
    iret