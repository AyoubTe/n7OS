#include <n7OS/cpu.h>
#include <inttypes.h>
#include <n7OS/processor_structs.h>
#include <n7OS/console.h>
#include <n7OS/paging.h>
#include <n7OS/mem.h>
#include <n7OS/kheap.h>

void kernel_start(void)
{
    setup_base(0 /* la memoire virtuelle n'est pas encore definie */);
    console_putbytes("Base setup OK\n", 15);

    init_console();
    console_putbytes("Booting...\n", 11);
    
    // Initialisation de la pagination
    // initialise_paging();
    // console_putbytes("Paging initialised!\n", 20);

    // // Test simple : allouer une page virtuelle et y écrire
    // uint32_t test_addr = 0x100000; // Adresse virtuelle exemple
    // alloc_page_entry(test_addr, 1, 1); // RW, kernel mode

    // uint32_t *test_page = (uint32_t*) test_addr;
    // *test_page = 0xDEADBEEF;  // On écrit une valeur test

    // if (*test_page == 0xDEADBEEF) {
    //     console_putbytes("Page mapping OK!\n", 18);
    // } else {
    //     console_putbytes("Page mapping FAILED!\n", 22);
    // }


    // lancement des interruptions
    sti();

    // on ne doit jamais sortir de kernel_start
    while (1) {
        // cette fonction arrete le processeur
        hlt();
    }
}
