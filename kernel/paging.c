#include <n7OS/paging.h>
#include <n7OS/mem.h>
#include <n7OS/kheap.h>
#include <n7OS/console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

PageDirectory pageDirectory;

/**
 * @brief Définit une entrée de table de pages avec les paramètres spécifiés.
 *
 * Cette fonction configure une entrée de table de pages (PTE) avec une nouvelle page,
 * et définit les attributs de l'entrée tels que l'accessibilité en écriture et le niveau de privilège.
 *
 * @param page_table_entry Pointeur vers l'entrée de la table de pages à configurer.
 * @param new_page Adresse de la nouvelle page à mapper (doit être alignée sur 4 Ko).
 * @param is_writeable Indicateur si la page doit être accessible en écriture (1 pour oui, 0 pour non).
 * @param is_kernel Indicateur si la page doit être accessible uniquement par le noyau (1 pour oui, 0 pour non).
 */
void setPageEntry(PTE *page_table_entry, uint32_t new_page, int is_writeable, int is_kernel) {
    page_table_entry->page_entry.P= 1;
    page_table_entry->page_entry.A= 0;
    page_table_entry->page_entry.D= 0;
    page_table_entry->page_entry.W= is_writeable;
    page_table_entry->page_entry.U= ~is_kernel;
    page_table_entry->page_entry.page= new_page>>12;
}

/**
 * @brief Initialise le système de pagination.
 *
 * Cette fonction initialise le système de pagination en allouant et en configurant
 * les structures de données nécessaires pour la pagination. Elle effectue les étapes suivantes :
 * 
 * 1. Initialise la mémoire en appelant init_mem().
 * 2. Alloue et initialise un répertoire de pages (PageDirectory) de 1024 entrées.
 * 3. Pour chaque entrée du répertoire de pages, alloue et initialise une table de pages (PageTable) de 1024 entrées.
 * 4. Configure chaque entrée du répertoire de pages pour pointer vers une table de pages avec les attributs Présent et RW.
 * 5. Effectue un mapping simple de la première zone mémoire (identité) en allouant des entrées de pages pour les adresses de 0 à 4 Mo.
 * 6. Charge le répertoire de pages en mémoire en appelant loadPageDirectory().
 * 7. Active la pagination en appelant enablePaging().
 */
void initialise_paging() {
    uint32_t indice = 0;

    init_mem();

    pageDirectory = (PageDirectory) kmalloc_a(sizeof(PDE) * 1024);
    if (!pageDirectory) {
        panic("Page directory null");
    }
    
    memset(pageDirectory, 0, sizeof(PDE) * 1024);
    for (int i = 0; i < 1024; i++) {
        PageTable new_page_table = (PageTable) kmalloc_a(sizeof(PTE) * 1024);
        if (!new_page_table || (uint32_t)new_page_table == 0) {
            panic("Page table null");
        }
        memset(new_page_table, 0, sizeof(PTE) * 1024);
        pageDirectory[i].value = (uint32_t)new_page_table | 1 | 2; // Présent + RW
        indice = (uint32_t) new_page_table + sizeof(PTE) * 1024;
    }

    for (uint32_t addr = 0; addr < indice; addr += PAGE_SIZE) {
        alloc_page_entry(addr, 1, 1);
    }

    loadPageDirectory((unsigned int *)pageDirectory);
    enablePaging();
}


/**
 * @brief Alloue une entrée de page dans la table des pages.
 *
 * Cette fonction alloue une entrée de page dans la table des pages pour une adresse donnée.
 * Si la table des pages n'existe pas encore, elle est allouée et initialisée.
 * Une page physique libre est ensuite trouvée et associée à l'entrée de page.
 *
 * @param address L'adresse virtuelle pour laquelle allouer une entrée de page.
 * @param is_writeable Indique si la page doit être accessible en écriture (1 pour oui, 0 pour non).
 * @param is_kernel Indique si la page est une page du noyau (1 pour oui, 0 pour non).
 * @return PageTable La table des pages contenant l'entrée allouée.
 *
 * @note Cette fonction déclenche une panique si aucune page physique libre n'est disponible.
 */
PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel) {
    // address = adresse virtuelle à allouer 
    // address = idx_PDE | idx_PTE | offset
    //             10    |    10   |   12

    // Calcul de la position de la table de page dans le répertoire de pages (10 bits de poids fort)
    int idx_pagedir = (address >> 22) & 0x3FF;

    PageTable page_table;

    // Récupération de l'entrée dans le répertoire de pages
    PDE page_dir_entry = pageDirectory[idx_pagedir];

    // Récupération de l'adresse de la table des pages
    if (page_dir_entry.value & 1) {
        // Si la table de pages existe déjà (bit présent = 1), on récupère son adresse
        page_table = (PageTable)(page_dir_entry.value & 0xFFFFF000);
    } else {
        // Sinon, on alloue une nouvelle table de pages
        page_table = (PageTable)kmalloc_a(sizeof(PTE) * 1024);
        if (page_table == NULL) {
            panic("Échec d'allocation de la table de pages");
        }
        memset(page_table, 0, sizeof(PTE) * 1024);

        // Mise à jour de l'entrée dans le répertoire de pages
        page_dir_entry.value = (uint32_t)page_table | 1 | 2; // Présent + RW
        pageDirectory[idx_pagedir] = page_dir_entry;
    }

    // Recherche d'une page physique libre
    uint32_t phy_page = findfreePage();
    if (phy_page == 0) {
        panic("Plus de pages physiques disponibles");
    }

    // Calcul de la position de la page dans la table de pages (bits 12-21)
    int idx_pagetab = (address >> 12) & 0x3FF;

    // Mise à jour de l'entrée dans la table de pages
    setPageEntry(&page_table[idx_pagetab], phy_page, is_writeable, is_kernel);

    return page_table;
}



/**
 * @brief Gestionnaire de défaut de page.
 *
 * Cette fonction est appelée lorsqu'un défaut de page se produit. Elle récupère
 * l'adresse fautive à partir du registre CR2, puis analyse le code d'erreur pour
 * déterminer la cause du défaut de page. Elle affiche ensuite des informations
 * détaillées sur le défaut de page et déclenche une panique système.
 *
 * @param reg Structure contenant les registres au moment du défaut de page.
 *
 * Codes d'erreur possibles :
 * - Bit 0 : Page non présente
 * - Bit 1 : Opération de lecture/écriture (0 = lecture, 1 = écriture)
 * - Bit 2 : Mode utilisateur/superviseur (0 = superviseur, 1 = utilisateur)
 * - Bit 3 : Réservé (toujours 0)
 * - Bit 4 : Instruction fetch (0 = non, 1 = oui)
 */
void handler_page_fault(registers_t reg) {
    uint32_t faulting_address;
    __asm__ __volatile__("mov %%cr2, %0" : "=r" (faulting_address));

    int present = !(reg.err_code & 0x1);
    int rw = reg.err_code & 0x2;
    int us = reg.err_code & 0x4;
    int reserved = reg.err_code & 0x8;
    int id = reg.err_code & 0x10;

    printf("Page fault at 0x%x: ", faulting_address);
    if (present) printf("present ");
    if (rw) printf("read-only ");
    if (us) printf("user-mode ");
    if (reserved) printf("reserved ");
    if (id) printf("instruction fetch ");
    printf("\n");

    panic("Page fault");
}


/**
 * @brief Charge le répertoire de pages spécifié dans le registre CR3.
 *
 * Cette fonction utilise une instruction d'assemblage inline pour charger
 * l'adresse du répertoire de pages dans le registre de contrôle CR3, ce qui
 * permet de changer le contexte de pagination.
 *
 * @param dir Pointeur vers le répertoire de pages à charger.
 */
void loadPageDirectory(unsigned int* dir) {
    __asm__ __volatile__("mov %0, %%cr3":: "r"(dir) : "memory");
}

/**
 * @brief Active la pagination en définissant le bit PG (bit 31) du registre CR0.
 *
 * Cette fonction lit la valeur actuelle du registre CR0, définit le bit 31
 * pour activer la pagination, puis écrit la nouvelle valeur dans le registre CR0.
 *
 * @note Cette opération est critique pour le fonctionnement du système d'exploitation
 * et doit être effectuée avec précaution.
 */
void enablePaging() {
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // Set bit 31 (PG - enable paging)
    __asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0));
}

