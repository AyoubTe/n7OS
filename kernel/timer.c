//
// Created by Ayoub SAMI on 03/04/2025.
//
#include <n7OS/timer.h>
#include <inttypes.h>
#include <n7OS/cpu.h>
#include <n7OS/processus.h>
#include <n7OS/irq.h>

void init_timer() {
    // Calcul de la fréquence : 1 kHz
    uint32_t freq = 1193180 / 1000;

    // Programmation du PIT : canal 0, mode 2 (rate generator), en mode binaire
    outb(0x34, 0x43);
    outb(freq & 0xFF, 0x40); // Octet de poids faible
    outb(freq >> 8, 0x40);   // Octet de poids fort

    // Démasquage de l'IRQ0 (timer) dans le PIC
    outb(inb(0x21) & ~1, 0x21);

    // Initialisation dans l'IDT de l'entrée pour le timer
    init_irq_entry(IRQ_TIMER, (uint32_t)timer_handler);
}

void timer_handler() {
    static uint32_t ticks = 0;
    ticks++;

    // Acquittement de l'interruption auprès du PIC
    outb(0x20, 0x20);

    // Appel de l'ordonnanceur pour passer à un autre processus
    schedule();
}
