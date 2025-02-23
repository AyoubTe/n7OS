#include <n7OS/paging.h>
#include <stddef.h> // nécessaire pour NULL

void initialise_paging() {

}

PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel) {
    PageTable pgtab= NULL;
    (void)address;
    (void)is_writeable;
    (void)is_kernel;

    return pgtab;
}
