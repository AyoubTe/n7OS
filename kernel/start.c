#include <n7OS/cpu.h>
#include <n7OS/processor_structs.h>
#include <n7OS/console.h>
#include <n7OS/paging.h>
#include <n7OS/kheap.h>
#include <n7OS/timer.h>
#include <n7OS/processus.h>
#include <stdio.h>
#include <inttypes.h>
#include <n7OS/irq.h>

extern void handler_irq50(void);

void kernel_start(void)
{
    // 1. Initialisation du processeur
    init_kheap();

    // 2. Initialisation de la console
    init_console();
    printf("La console est prete!\n");

    // 3. Initialisation de la paging
    uint32_t base = (uint32_t)initialise_paging();
    printf("La pagination est initialisee avec base %x\n", base);
    /* la memoire virtuelle est definie */
    setup_base((int)base);

    // 4. Test memory mapping
    uint32_t test_addr = 0x500000;
    uint32_t *test_page = (uint32_t*)test_addr;
    (*test_page) = 56;
    if (*test_page == 56) {
         printf("Le mapping de la page est effectue avec succes!\n");
    } else {
         printf("Le mapping de la page est echoue!\n");
    }

    // 5. Initialisation du timer
    init_timer();
    printf("Le timer est initialise!\n");

    // 6. Test des processus
    //test_processus();

    // 7. Test des interruptions
    // Initialisation de l'entrée d'interruption 50
    init_irq_entry(50, (uint32_t)handler_irq50);

    // lancement des interruptions
    sti();
    printf("Les interruptions sont lancees!\n");

    // Déclenchement de l'interruption 50
    __asm__("int $50");

    // Déclenchement de l'interruption 0 de timer
    __asm__("int $0");

    // on ne doit jamais sortir de kernel_start
    while (1) {
        // cette fonction arrete le processeur
        hlt();
    }
}
