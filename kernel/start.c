#include <n7OS/cpu.h>
#include <n7OS/processor_structs.h>
#include <n7OS/console.h>
#include <n7OS/paging.h>
#include <n7OS/kheap.h>
#include <stdio.h>
#include <inttypes.h>

void kernel_start(void)
{
    init_kheap();
    init_console();
    uint32_t base = initialise_paging();
    /* la memoire virtuelle est definie */
    setup_base(base);
    
    printf("Hello, world from kernel\n");

    // 4. Test memory mapping
    uint32_t test_addr = 0x500000; // Test address beyond initial mapping
    alloc_page_entry(test_addr, 1, 1);
    uint32_t *test_page = (uint32_t*)test_addr;
    *test_page = 0xDEADBEEF;
    if (*test_page == 0xDEADBEEF) {
         console_putbytes("Page mapping OK!\n", 18);
    } else {
         console_putbytes("Page mapping FAILED!\n", 22);
    }

    // lancement des interruptions
    sti();

    // on ne doit jamais sortir de kernel_start
    while (1) {
        // cette fonction arrete le processeur
        hlt();
    }
}
