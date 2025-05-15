/**
 * @file paging.h
 * @brief Gestion de la pagination dans le noyau
 */
#ifndef _PAGING_H
#define _PAGING_H

#include <inttypes.h>

extern void loadPageDirectory(unsigned int* dir);
extern void enablePaging();

#define PAGE_PRESENT 1
#define PAGE_RW      2
#define PAGE_USER    4
#define PAGE_DIRTY   16

/**
 * @brief Description d'une ligne de la table de page
 * 
 */
typedef struct {
    uint32_t present     : 1; // indique si la page est dans la mémoire physique (1) ou non (0)
    uint32_t rw          : 1; // Indique il est Read-Only (0) ou Read-Write (1)
    uint32_t user        : 1; // Indique si la page est accessible par le noyau (0) ou par l'utilisateur (1)
    uint32_t accessed    : 1; // Indique si la page a été accédée
    uint32_t dirty       : 1; // Indique la page a été écrite dès la derniere fois
    uint32_t unused      : 7; //
    uint32_t page        : 20; // Adresse de la page en mémoire physique (20 bits), adresse de la page cadre physique (bits de poids fort)
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
 * @brief Structure décrivant une entrée du répertoire de pages
 * 
 * Cette structure est utilisée pour décrire les entrées dans le répertoire de pages (Page Directory).
 * Une entrée contient des informations sur la présence, les droits d'accès et l'emplacement
 * de la table de pages associée.
 */
typedef struct {
    uint32_t present     : 1; /**< Indique si la table de pages est présente en mémoire physique (1) ou non (0) */
    uint32_t rw          : 1; /**< Indique si les pages associées sont en lecture seule (0) ou lecture/écriture (1) */
    uint32_t user        : 1; /**< Indique si les pages associées sont accessibles par le noyau (0) ou par l'utilisateur (1) */
    uint32_t reserved    : 9; /**< Bits réservés, non utilisés */
    uint32_t page_table  : 20; /**< Adresse physique de la table de pages (bits de poids fort) */
} page_dir_entry_t;

/**
 * @union PDE
 * @brief Une entrée du répertoire de pages peut être manipulée en utilisant
 *        la structure page_dir_entry_t ou directement la valeur.
 */
typedef union {
    page_dir_entry_t dir_entry; /**< Structure décrivant une entrée du répertoire de pages */
    uint32_t value;             /**< Valeur brute associée à l'entrée du répertoire de pages */
} PDE;

typedef PDE * PageDir;

/**
 * @brief Cette fonction initialise le répertoire de page, alloue les pages de table du noyau
 *        et active la pagination
 * 
 */
PageDir initialise_paging();

/**
 * @brief Cette fonction alloue une page de la mémoire physique à une adresse de la mémoire virtuelle
 * 
 * @param address       Adresse de la mémoire virtuelle à mapper
 * @param is_writeable  Si is_writeable == 1, la page est accessible en écriture
 * @param is_kernel     Si is_kernel == 1, la page ne peut être accédée que par le noyau
 * @return PageTable    La table de page modifiée
 */
PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel);

void setPageEntry(PTE *page_table_entry, uint32_t new_page, int is_writeable, int is_kernel);

typedef struct {
    uint32_t ds;                                     // Segment data
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Poussés par pusha
    uint32_t int_no, err_code;                       // Numéro d'interruption et code d'erreur
    uint32_t eip, cs, eflags, useresp, ss;           // Poussés par le processeur automatiquement
} registers_t;

void handler_page_fault(registers_t reg);

#endif