.text
.globl keyboard_handler
keyboard_handler:
    // Sauvegarder les registres
    pusha
    
    // Appeler la fonction C de traitement
    call keyboard_interrupt
    
    // Restaurer les registres
    popa
    iret