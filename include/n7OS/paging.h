/**
 * @file paging.h
 * @brief Gestion de la pagination dans le noyau
 */
#ifndef _PAGING_H
#define _PAGING_H

#include <inttypes.h>

extern void loadPageDirectory(unsigned int* dir);
extern void enablePaging();

/**
 * @brief Description d'une ligne de la table de page
 * 
 */
typedef struct {
    uint8_t P: 1; // P=1 : page présente en mémoire
    uint8_t W: 1; // W=1 : page accessible en lecture/écriture (0 : lecture seule)
    uint8_t U: 1; // U=1 : page utilisateur; U=0 : page en mode noyau
    uint8_t RSVD: 2; // RSVD : bits réservés
    uint8_t A: 1; // A : Accessed bit
    uint8_t D: 1; // D : Dirty bit (page modifiée)
    uint8_t RSVD2: 2; // RSVD : bits réservés
    uint8_t AVAIL: 3; 
    uint32_t page: 20; // Page : Adresse de la page en mémoire physique (20 bits)
    
} page_table_entry_t;

/**
 * @brief Une entrée dans la table de page peut être manipulée en utilisant
 *        la structure page_table_entry_t ou directement la valeur
 */
typedef union {
    page_table_entry_t page_entry;
    uint32_t value;
} PTE; // PTE = Page Table Entry 

/**
 * @brief Une table de page (PageTable) est un tableau de descripteurs de page
 * 
 */
typedef PTE * PageTable;

/**
 * @brief Cette fonction initialise le répertoire de page, alloue les pages de table du noyau
 *        et active la pagination
 * 
 */
void initialise_paging();

/**
 * @brief Cette fonction alloue une page de la mémoire physique à une adresse de la mémoire virtuelle
 * 
 * @param address       Adresse de la mémoire virtuelle à mapper
 * @param is_writeable  Si is_writeable == 1, la page est accessible en écriture
 * @param is_kernel     Si is_kernel == 1, la page ne peut être accédée que par le noyau
 * @return PageTable    La table de page modifiée
 */
PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel);


/**
 * @struct page_dir_entry_t
 * @brief Représente une entrée de répertoire de pages pour la pagination.
 *
 * Cette structure est utilisée pour définir une entrée dans le répertoire de pages,
 * qui est une partie essentielle de la gestion de la mémoire virtuelle.
 *
 * @var page_dir_entry_t::P
 * @brief Indicateur de présence.
 *        - 1 : La table de pages est présente en mémoire.
 *        - 0 : La table de pages n'est pas présente en mémoire.
 *
 * @var page_dir_entry_t::W
 * @brief Indicateur de lecture/écriture.
 *        - 1 : La table de pages est accessible en lecture/écriture.
 *        - 0 : La table de pages est accessible en lecture seule.
 *
 * @var page_dir_entry_t::U
 * @brief Indicateur de niveau de privilège.
 *        - 1 : La page est accessible en mode utilisateur.
 *        - 0 : La page est accessible uniquement en mode noyau (superviseur).
 *
 * @var page_dir_entry_t::Reserved
 * @brief Champs réservés.
 *        - Doit être mis à zéro.
 *
 * @var page_dir_entry_t::page_table
 * @brief Adresse de la table de pages en mémoire physique.
 *        - Contient les 20 bits les plus significatifs de l'adresse physique de la table de pages.
 */
typedef struct {
    uint8_t P: 1;   // P=1 : table de page présente en mémoire
    uint8_t W: 1;   // W=1 : table de page accessible en lecture/écriture (0 : lecture seule)
    uint8_t U: 1;   // Supervisor level only if clear
    uint16_t Reserved: 9;   // U=1 : page utilisateur; U=0 : page en mode noyau
    uint32_t page_table: 20;  // Page : Adresse de la page en mémoire physique (20 bits)
} page_dir_entry_t;

/**
 * @union PDE
 * @brief Union representing a page directory entry.
 * 
 * This union allows access to a page directory entry either as a 
 * structured `page_dir_entry_t` or as a raw 32-bit value.
 * 
 * @var PDE::dir_entry
 * Member representing the page directory entry as a structured type.
 * 
 * @var PDE::value
 * Member representing the page directory entry as a raw 32-bit value.
 */

/**
 * @union PDE
 * @brief Union représentant une entrée de répertoire de pages.
 * 
 * Cette union permet d'accéder à une entrée de répertoire de pages soit 
 * comme un type structuré `page_dir_entry_t`, soit comme une valeur brute de 32 bits.
 * 
 * @var PDE::dir_entry
 * Membre représentant l'entrée de répertoire de pages comme un type structuré.
 * 
 * @var PDE::value
 * Membre représentant l'entrée de répertoire de pages comme une valeur brute de 32 bits.
 */
typedef union {
    page_dir_entry_t dir_entry;
    uint32_t value;
} PDE;

/**
 * @brief Un répertoire de pages (PageDirectory) est un tableau de descripteurs de table de pages
 * 
 */
typedef PDE * PageDirectory;


/**
 * @brief Structure représentant les registres du processeur lors d'une interruption.
 *
 * Cette structure est utilisée pour sauvegarder l'état des registres du processeur
 * lorsqu'une interruption se produit. Elle contient les registres segmentaires, 
 * les registres généraux, ainsi que les informations spécifiques à l'interruption.
 *
 * @struct registers_t
 * @var registers_t::ds
 * Segment de données.
 * @var registers_t::edi
 * Registre EDI.
 * @var registers_t::esi
 * Registre ESI.
 * @var registers_t::ebp
 * Registre EBP.
 * @var registers_t::esp
 * Registre ESP.
 * @var registers_t::ebx
 * Registre EBX.
 * @var registers_t::edx
 * Registre EDX.
 * @var registers_t::ecx
 * Registre ECX.
 * @var registers_t::eax
 * Registre EAX.
 * @var registers_t::int_no
 * Numéro de l'interruption.
 * @var registers_t::err_code
 * Code d'erreur de l'interruption.
 * @var registers_t::eip
 * Pointeur d'instruction.
 * @var registers_t::cs
 * Segment de code.
 * @var registers_t::eflags
 * Registre EFLAGS.
 * @var registers_t::useresp
 * Pointeur de pile utilisateur.
 * @var registers_t::ss
 * Segment de pile.
 */
typedef struct {
    uint32_t ds;                                     // Segment data
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Poussés par pusha
    uint32_t int_no, err_code;                       // Numéro d'interruption et code d'erreur
    uint32_t eip, cs, eflags, useresp, ss;           // Poussés par le processeur automatiquement
} registers_t;

#endif