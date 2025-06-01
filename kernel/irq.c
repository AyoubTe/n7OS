#include <inttypes.h>
#include <stdio.h>
#include <n7OS/irq.h>
#include <n7OS/cpu.h>
#include <n7OS/segment.h>
#include <n7OS/processor_structs.h>

// Chaque entrée sera traitée comme une structure idt_entry_t.
void init_irq_entry(int irq_num, uint32_t addr)
{
    idt_entry_t *entry = (idt_entry_t *)&idt[irq_num];

    // Découper l'adresse de la routine d'interruption en deux parties
    entry->offset_inf = addr & 0xFFFF;                 // octets bas de l'adresse
    entry->sel_segment = KERNEL_CS;                     // On utilise le sélecteur défini (à adapter si besoin)
    entry->zero = 0;
    entry->type_attr = 0x8E; // Pour autoriser les appels depuis le mode utilisateur type_attr = 0xEE; // DPL=3 (0xEE = 1110 1110 en binaire                        // Présent, DPL=0, type « interrupt gate » 32 bits
    entry->offset_sup = (addr >> 16) & 0xFFFF;          // octets hauts de l'adresse
}

void handler_IT50_C(void)
{
    // Affiche un message indiquant que l'interruption est bien reçue
    printf("Interruption 50 recue !\n");
}
