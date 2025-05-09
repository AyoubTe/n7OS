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

// // fonction init_pic() pour remapper les interruptions matérielles
void init_pic() {
    // ICW1 : démarrage, cascade
    outb(0x11, 0x20);
    outb(0x11, 0xA0);

    // ICW2 : vecteur de base
    outb(0x20, 0x21);  // PIC maître → vecteurs 0x20–0x27
    outb(0x28, 0xA1);  // PIC esclave → vecteurs 0x28–0x2F

    // ICW3 : configuration cascade
    outb(0x04, 0x21);  // maître: IRQ2 → esclave
    outb(0x02, 0xA1);  // esclave : cascade ID = 2

    // ICW4 : mode 8086
    outb(0x01, 0x21);
    outb(0x01, 0xA1);

    // Masquer toutes sauf IRQ0 (timer) sur maître, tout sur esclave
    outb(0xFE, 0x21);
    outb(0xFF, 0xA1);
}
