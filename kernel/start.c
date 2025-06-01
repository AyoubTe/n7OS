
#include <n7OS/cpu.h>
#include <n7OS/processor_structs.h>
#include <n7OS/console.h>
#include <n7OS/paging.h>
#include <n7OS/kheap.h>
#include <n7OS/timer.h>
#include <n7OS/processus.h>
#include <n7OS/irq.h>
#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <n7OS/keyboard.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>


extern void handler_irq50(void);

extern void shell_loop(void);

void kernel_start(void)
{
    // 1. Initialisations
    init_kheap();
    

    // 2. Initialisation de la paging
    uint32_t base = (uint32_t)initialise_paging();
    // printf("La pagination est initialisee avec base %x\n", base);
    /* la memoire virtuelle est definie */
    setup_base((int)base);

    init_processus();

    // 3. Interruptions
    // Initialisation de l'entrée d'interruption 50
    init_irq_entry(50, (uint32_t)handler_irq50);

    init_syscall();

    // lancement des interruptions
    sti();

    // printf("Les interruptions sont lancees!\n");

    // Déclenchement de l'interruption 50
    //__asm__("int $50");

    // Déclenchement de l'interruption 0 de timer
    //__asm__("int $0");

    // 4. Initialisation de la console
    init_console();
    printf("Welcome to n7OS!!\n");

    // 5. Test memory mapping
    // uint32_t test_addr = 0xA500000;
    // uint32_t *test_page = (uint32_t*)test_addr;
    // (*test_page) = 56;
    // if (*test_page == 56) {
    //     printf("Le mapping de la page est effectue avec succes!\n");
    // } else {
    //     printf("Le mapping de la page est echoue!\n");
    // }
    

    // 6. Test des appels systèmes
    // Test 1: sys_example
    // Test 1: sys_example avec plus de détails
    // int result;
    // result = example();
    
    // if (result == 1) {
    //     printf("Test 1: Appel systeme 'example' reussi\n");
    // } else {
    //     printf("Test 1: Echec de l'appel systeme 'example'\n");
    // }

    // 7. Test des processus
    //test_processus();


    // 8. Test clavier
    // Initialisation du clavier
    init_keyboard();
    // printf("Initialisation du clavier effectuée.\n");

    // // Test de lecture clavier
    // printf("Tapez des caractères (Entrée pour terminer): ");
    // char c;
    // while ((c = kgetch()) != '\n') {
    //     printf("%s", &c);
    // }

    // printf("\nTest clavier termine.\n");

    // Créer le shell au démarrage
    creer_p("shell", shell_loop);
    //pid_t shell_pid = creer_p("shell", shell_loop);
    //if ((int)shell_pid > 0) process_table[shell_pid].priority = 0;  // priorité plus grand que le kernel

    // on ne doit jamais sortir de kernel_start
    while (1) {
        // cette fonction arrete le processeur
        hlt();
    }
}
