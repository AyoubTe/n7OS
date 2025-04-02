#include <n7OS/mem.h>
#include <string.h>
#include <stdio.h>

#define BITMAP_SIZE (LAST_MEMORY_INDEX / PAGE_SIZE / 8)

static uint8_t bitmap[BITMAP_SIZE]; // Bitmap pour suivre l'état des pages

/**
 * @brief Marque la page allouée
 * 
 * Lorsque la page a été choisie, cette fonction permet de la marquer allouée
 * 
 * @param addr Adresse de la page à allouer
 */
void setPage(uint32_t addr) {
    uint32_t pageIndex = addr / PAGE_SIZE;
    bitmap[pageIndex / 8] |= (1 << (pageIndex % 8));
}

/**
 * @brief Désalloue la page
 * 
 * Libère la page allouée.
 * 
 * @param addr Adresse de la page à libérer
 */
void clearPage(uint32_t addr) {
    uint32_t pageIndex = addr / PAGE_SIZE;
    bitmap[pageIndex / 8] &= ~(1 << (pageIndex % 8));
}

/**
 * @brief Fourni la première page libre de la mémoire physique tout en l'allouant
 * 
 * @return uint32_t Adresse de la page sélectionnée
 */
uint32_t findfreePage() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] != 0xFF) { // Si un octet contient une page libre
            for (uint8_t j = 0; j < 8; j++) {
                if (!(bitmap[i] & (1 << j))) { // Une page libre a été trouvée
                    uint32_t freePage = (i * 8 + j) * PAGE_SIZE;
                    setPage(freePage);
                    return freePage;
                }
            }
        }
    }
    return 0; // Pas de page libre trouvée
}

/**
 * @brief Initialise le gestionnaire de mémoire physique
 * 
 */
void init_mem() {
    memset(bitmap, 0, sizeof(bitmap)); // Initialise le bitmap à 0 (aucune page allouée)
}

/**
 * @brief Affiche l'état de la mémoire physique
 * 
 */
void print_mem() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            uint32_t pageAddr = (i * 8 + j) * PAGE_SIZE;
            if (bitmap[i] & (1 << j)) {
                printf("Page 0x%08X: Allouée\n", pageAddr);
            } else {
                printf("Page 0x%08X: Libre\n", pageAddr);
            }
        }
    }
}