#include <n7OS/paging.h>
#include <n7OS/kheap.h>
#include <n7OS/mem.h>
#include <stddef.h>
#include <stdio.h>

/// Répertoire de pages du noyau utilisé pour la gestion de la pagination
static PageDir directory;

/**
 * @brief Initialise la pagination en créant le répertoire de pages, en configurant
 *        un mapping identité pour la zone du noyau et en activant la pagination.
 */
PageDir initialise_paging() {
    int i, j;

    /* Allocation du répertoire de pages (1024 entrées) avec alignement sur page */
    directory = (PageDir) kmalloc_a(1024 * sizeof(PDE));
    for (i = 0; i < 1024; i++) {
        directory[i].value = 0;
    }

    /* Création et initialisation des tables de pages pour un mapping identité.
       Ici, on mappe les 4 Mo inférieurs, ce qui correspond à 1024 tables
       de pages (chaque table contient 1024 entrées de 4 Ko). */
    for (i = 0; i < 1024; i++) {
        PageTable pgtab = (PageTable) kmalloc_a(1024 * sizeof(PTE));
        for (j = 0; j < 1024; j++) {
            /* On assigne l'adresse physique correspondant à l'entrée et on
               active la page en lecture/écriture (pour le noyau, PAGE_USER non défini) */
            pgtab[j].value = ((i * 1024 + j) * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
        }
        /* L'entrée du répertoire de pages pointe vers la table nouvellement créée */
        directory[i].value = ((uint32_t)pgtab & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;
    }

    // register_interrupt_handler(14, handler_page_fault);

    /* Charger le répertoire de pages et activer la pagination */
    loadPageDirectory((unsigned int*) directory);
    enablePaging();

    /* Retourner le répertoire de tables de pages */
    return directory;

}

/**
 * @brief Alloue et configure une entrée de page dans la table de pages associée à l'adresse virtuelle.
 *
 * Cette fonction cherche ou crée la table de pages correspondante dans le répertoire
 * et alloue une page physique à l'adresse virtuelle donnée.
 *
 * @param address     Adresse virtuelle à mapper.
 * @param is_writeable Si 1, la page est accessible en écriture ; sinon en lecture seule.
 * @param is_kernel   Si 1, la page est réservée au noyau ; sinon, accessible par l'utilisateur.
 * @return PageTable La table de pages modifiée.
 */
PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel) {
    uint32_t dir_index = address >> 22;              // indice dans le répertoire
    uint32_t table_index = (address >> 12) & 0x03FF;   // indice dans la table

    PageTable pgtab = NULL;

    /* Vérifier si une table de pages existe déjà pour cette adresse */
    if (!(directory[dir_index].value & PAGE_PRESENT)) {
        /* Allocation d'une nouvelle table de pages alignée sur une page */
        pgtab = (PageTable) kmalloc_a(1024 * sizeof(PTE));
        for (int i = 0; i < 1024; i++) {
            pgtab[i].value = 0;
        }
        /* Mettre à jour l'entrée du répertoire avec la nouvelle table */
        directory[dir_index].value =
            ((uint32_t)pgtab & 0xFFFFF000) | PAGE_PRESENT |
            (is_writeable ? PAGE_RW : 0) | (is_kernel ? 0 : PAGE_USER);
    } else {
        /* La table existe déjà : on la récupère */
        pgtab = (PageTable) (directory[dir_index].value & 0xFFFFF000);
    }

    /* Allocation d'une page physique libre et marquage de celle-ci */
    uint32_t phys_page = findfreePage();
    setPage(phys_page);

    /* Mise à jour de l'entrée de la table de pages pour mapper l'adresse virtuelle */
    pgtab[table_index].value =
        (phys_page & 0xFFFFF000) | PAGE_PRESENT |
        (is_writeable ? PAGE_RW : 0) | (is_kernel ? 0 : PAGE_USER);

    return pgtab;
}


/**
 * @brief Sets up a page table entry with the specified attributes.
 *
 * This function initializes a page table entry with the given page address,
 * write permissions, and privilege level (kernel or user).
 *
 * @param page_table_entry Pointer to the page table entry (PTE) to be configured.
 * @param new_page The physical address of the new page to be mapped. The address
 *                 is shifted right by 12 bits to fit into the page table entry.
 * @param is_writeable A flag indicating whether the page should be writable (1 for writable, 0 for read-only).
 * @param is_kernel A flag indicating whether the page is for kernel mode (1 for kernel, 0 for user mode).
 *
 * @note The `present` field of the page table entry is set to 1, indicating that
 *       the page is present in memory. The `accessed` and `dirty` fields are
 *       initialized to 0.
 */
void setPageEntry(PTE *page_table_entry, uint32_t new_page, int is_writeable, int is_kernel) {
    page_table_entry->page_entry.present= 1;
    page_table_entry->page_entry.accessed= 0;
    page_table_entry->page_entry.dirty= 0;
    page_table_entry->page_entry.rw= is_writeable;
    page_table_entry->page_entry.user = is_kernel ? 0 : 1;
    page_table_entry->page_entry.page= new_page>>12;
}

/**
 * Charge le répertoire de pages dans le registre CR3
 * @param page_directory Adresse physique du répertoire de pages
 */
void loadPageDirectory(uint32_t* page_directory) {
    __asm__ volatile("mov %0, %%cr3" : : "r" (page_directory));
}

/**
 * Active la pagination en définissant le bit PG (bit 31) dans le registre CR0
 */
void enablePaging() {
    uint32_t cr0;
    __asm__ volatile(
        "mov %%cr0, %0\n"
        "or $0x80000000, %0\n"
        "mov %0, %%cr0"
        : "=r" (cr0)
        :
        : "memory"
    );
}


/**
 * @brief Gère une exception de défaut de page (page fault).
 *
 * Cette fonction est appelée lorsqu'un défaut de page se produit. Elle récupère
 * l'adresse fautive à l'origine du défaut de page et analyse le code d'erreur
 * pour déterminer la cause du défaut. Ensuite, elle affiche des informations
 * détaillées sur le défaut de page et déclenche un arrêt du système via la
 * fonction panic.
 *
 * @param reg Structure contenant les registres au moment de l'exception.
 *            Le champ `err_code` de cette structure contient le code d'erreur
 *            associé au défaut de page.
 *
 * @details
 * Le code d'erreur est analysé pour identifier les causes possibles :
 * - Bit 0 : La page n'est pas présente en mémoire (0 si absente).
 * - Bit 1 : Défaut causé par une tentative d'écriture sur une page en lecture seule.
 * - Bit 2 : Défaut survenu en mode utilisateur (1 si mode utilisateur).
 * - Bit 3 : Bits réservés écrasés dans l'entrée de la table de pages.
 * - Bit 4 : Défaut causé par une tentative de fetch d'instruction.
 *
 * La fonction affiche un message décrivant les causes possibles du défaut de page
 * ainsi que l'adresse fautive. Enfin, elle appelle la fonction `panic` pour
 * arrêter le système.
 */
void handler_page_fault(registers_t reg) {
    uint32_t faulting_address;
    __asm__ __volatile__("mov %%cr2, %0" : "=r" (faulting_address));

    int present  = !(reg.err_code & 0x1); // Page non présente si 0
    int rw       = reg.err_code & 0x2;      // Écriture ?
    int us       = reg.err_code & 0x4;      // Mode utilisateur ?
    int reserved = reg.err_code & 0x8;      // Bits réservés écrasés ?
    int id       = reg.err_code & 0x10;     // Défaut lors d'un fetch d'instruction ?

    printf("Page fault! ( ");
    if (present)  { printf("present "); }
    if (rw)       { printf("read-only "); }
    if (us)       { printf("user-mode "); }
    if (reserved) { printf("reserved "); }
    if (id)       { printf("instruction "); }
    printf(") at 0x%x\n", faulting_address);

    panic("Page fault");
}