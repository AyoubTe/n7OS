#include <n7OS/mem.h>
#include <stdio.h>
#include <string.h>

// Nombre maximal des pages
#define MAX_PAGES (LAST_MEMORY_INDEX / PAGE_SIZE)

// une bitmap pour savoir quelles pages sont libres ou allouées
static uint32_t free_page_bitmap[MAX_PAGES / 32];


/**
 * @brief Marque la page allouée
 * 
 * Lorsque la page a été choisie, cette fonction permet de la marquer allouée
 * 
 * @param addr Adresse de la page à allouer
 */
void setPage(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    free_page_bitmap[page / 32] |= (1 << (page % 32));
}

/**
 * @brief Désalloue la page
 * 
 * Libère la page allouée.
 * 
 * @param addr Adresse de la page à libérer
 */
void clearPage(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    free_page_bitmap[page / 32] &= ~(1 << (page % 32));
}

/**
 * @brief Fourni la première page libre de la mémoire physique tout en l'allouant
 * 
 * @return uint32_t Adresse de la page sélectionnée
 */
uint32_t findfreePage() {
    for (uint32_t i = 0; i < MAX_PAGES / 32; i++) {
        if (free_page_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(free_page_bitmap[i] & (1 << j))) {
                    uint32_t page_addr = (i * 32 + j) * PAGE_SIZE;
                    setPage(page_addr);
                    printf("Found free page at 0x%x\n", page_addr);
                    return page_addr;
                }
            }
        }
    }
    printf("No free pages available\n");
    return 0; // Plus de pages libres
}

/**
 * @brief Initialise le gestionnaire de mémoire physique
 * 
 */
void init_mem() {
    memset(free_page_bitmap, 0, sizeof(free_page_bitmap));
    printf("Memory initialized successfully\n");
}

/**
 * @brief Affiche l'état de la mémoire physique
 * 
 */
void print_mem() {
    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        if (free_page_bitmap[i / 32] & (1 << (i % 32))) {
            printf("Page %u allouée\n", i);
        }
    }
}