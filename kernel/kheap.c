#include <n7OS/kheap.h>
#include <stdio.h>

/**
 * @brief Lorsque tout le code du noyau est compilé, le tas du noyau commence juste après.
 * 
 */
extern uint32_t mem_heap;
uint32_t placement_address = (uint32_t)&mem_heap;

uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phys) {
    uint32_t address;
    if (align == 1 && (placement_address & 0xFFF) ) {
        // Aligne l'adresse si on n'est pas au début d'une page
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    if (phys) {
        *phys = placement_address;
    }
    address= placement_address;
    placement_address += sz;
    return address;
}

uint32_t kmalloc_a(uint32_t sz) {
    if (sz == 0) {
        printf("Error: kmalloc_a called with size 0\n");
        return 0;
    }
    
    // Aligner l'adresse sur une page (4 Ko)
    if (placement_address & 0xFFF) {
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }

    // Vérifier que l'adresse n'est pas 0x0
    if (placement_address == 0) {
        placement_address = 0x10000;  // Adresse de remplacement
    }

    uint32_t address = placement_address;
    placement_address += sz;

    return address;
}



uint32_t kmalloc_p(uint32_t sz, uint32_t *phys) {
    return kmalloc_int(sz, 0, phys);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys) {
    return kmalloc_int(sz, 1, phys);
}

uint32_t kmalloc(uint32_t sz) {
    return kmalloc_int(sz, 0, 0);
}
