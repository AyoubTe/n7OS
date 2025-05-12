//
// Created by Ayoub SAMI on 03/04/2025.
//

#include <n7OS/timer.h>
#include <inttypes.h>
#include <n7OS/cpu.h>
#include <n7OS/processus.h>
#include <n7OS/irq.h>
#include <n7OS/console.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern void timer_handler();

volatile uint32_t ticks = 0;

void init_timer() {
    // Calcul de la fréquence : 1 kHz
    uint32_t freq = FREQUENCE_SYSTEM / TIMER_FREQUENCY;

    // Programmation du PIT : canal 0, mode 2 (rate generator), en mode binaire
    outb(0x34, 0x43);
    // Octet de poids faible
    outb(freq & 0xFF, 0x40);
    // Octet de poids fort
    outb(freq >> 8, 0x40);   

    // Initialisation dans l'IDT de l'entrée pour le timer
    init_irq_entry(IRQ_TIMER, (uint32_t)timer_handler);

    // Démasquer IRQ0 (bit 0) dans le registre de masque du PIC maître
    outb(inb(0x21) & ~0x01, 0x21);
}

void timer_handler_c() {
    ticks++;

    // Acquittement de l'interruption auprès du PIC
    outb(0x20, 0x20);

    // Rafraîchir la date qu’une fois par minute
    // static uint8_t last_min = 255;
    // uint8_t h, m, s;
    // ticks_to_time(ticks, &h, &m, &s);
    // if (m != last_min) {
    //     display_date();
    //     last_min = m;
    // }

    // Affichage de la date
    display_date();

    // Affichage du temps
    //display_uptime();
    display_uptime_animated();

    // Appel de l'ordonnanceur pour passer à un autre processus
    if(ticks%20 == 0) { // Appel de shedule chaque 20 tops d'horloge
        schedule();
    }
}

// Convert system ticks to hh:mm:ss format
void ticks_to_time(uint32_t ticks, uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    uint32_t total_seconds = ticks / TIMER_FREQUENCY;

    *hours = total_seconds / 3600;
    *minutes = (total_seconds % 3600) / 60;
    *seconds = total_seconds % 60;
}


/**
 * @brief Lit un octet dans le CMOS.
 * @param reg Registre 0x00–0x09 (secondes, minutes, heures, jour, mois, année…).
 */
static uint8_t cmos_read(uint8_t reg) {
    outb((reg & 0x7F) | 0x80, 0x70);  // désactive NMI, sélectionne reg
    uint8_t val = inb(0x71);
    return val;
}


static uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

bool rtc_get_datetime(rtc_time_t *dt) {
    // Attendre que l’UPDATE-IN-PROGRESS soit 0 pour avoir des valeurs cohérentes
    while (cmos_read(0x0A) & 0x80);
    dt->sec   = bcd_to_bin(cmos_read(0x00));
    dt->min   = bcd_to_bin(cmos_read(0x02));
    dt->hour  = bcd_to_bin(cmos_read(0x04));
    dt->day   = bcd_to_bin(cmos_read(0x07));
    dt->month = bcd_to_bin(cmos_read(0x08));
    dt->year  = bcd_to_bin(cmos_read(0x09));
    return true;
}

void display_date() {
    rtc_time_t now;
    if (!rtc_get_datetime(&now)) return;

    char date_str[16];
    snprintf(date_str, sizeof(date_str),
             "%02u/%02u/20%02u",
             now.day, now.month, now.year);

    int start_col = VGA_WIDTH - strlen(date_str) - 12; // à gauche de l’horloge
    for (int i = 0; i < (int)strlen(date_str); i++) {
        console_putchar_at(0, start_col + i, date_str[i]);
    }
}


// Display the system uptime in hh:mm:ss format
void display_uptime() {
    uint8_t hours, minutes, seconds;
    ticks_to_time(ticks, &hours, &minutes, &seconds);
    
    // Positionner l'heure en haut à droite (colonne 66 sur 80)
    char time_str[12];
    snprintf(time_str, sizeof(time_str), " %02u:%02u:%02u ", hours, minutes, seconds);
    
    // Écrire chaque caractère à sa position
    int start_col = VGA_WIDTH - strlen(time_str);
    for (int i = 0; i < (int)strlen(time_str); i++) {
        console_putchar_at(0, start_col + i, time_str[i]);
    }
}



// Améloirations
/*

// Par exemple pour l’heure :
for (int i = 0; i < (int)strlen(time_str); i++) {
    console_putchar_at_attr(0, start_col + i, time_str[i], 0x1E);
}

// Et pour la date en gris clair sur fond magenta :
for (int i = 0; i < (int)strlen(date_str); i++) {
    console_putchar_at_attr(0, start_col_date + i, date_str[i], 0x8D);
}

*/


void display_uptime_boxed() {
    char buf[20];
    uint8_t h, m, s;
    ticks_to_time(ticks, &h, &m, &s);
    snprintf(buf, sizeof(buf), " %02u:%02u:%02u ", h, m, s);
    int start = VGA_WIDTH - strlen(buf) - 2;

    // Coins et traits : ┌─┐ et │ │ et └─┘ (codes CP437 0xC9,0xCD,0xBB,0xBA,0xC8,0xBC)
    console_putchar_at_attr(0, start,   0xC9, 0x2F);         // ┌
    for (int i = 0; i < (int)strlen(buf); i++)
        console_putchar_at_attr(0, start+1+i, 0xCD, 0x2F);    // ─
    console_putchar_at_attr(0, start+1+strlen(buf), 0xBB, 0x2F); // ┐

    console_putchar_at_attr(1, start,   0xBA, 0x2F);         // │
    for (int i = 0; i < (int)strlen(buf); i++)
        console_putchar_at_attr(1, start+1+i, buf[i], 0x2F);  // texte
    console_putchar_at_attr(1, start+1+strlen(buf), 0xBA, 0x2F);// │

    console_putchar_at_attr(2, start,   0xC8, 0x2F);         // └
    for (int i = 0; i < (int)strlen(buf); i++)
        console_putchar_at_attr(2, start+1+i, 0xCD, 0x2F);    // ─
    console_putchar_at_attr(2, start+1+strlen(buf), 0xBC, 0x2F);// ┘
}

// Affichage animé pour l'heure en changant la couleur selon l'heure
void display_uptime_animated() {
    uint8_t color = (ticks / TIMER_FREQUENCY) % 2
                    ? 0xE1  // jaune sur bleu
                    : 0x2F; // vert sur bleu
    // même principe que display_uptime, mais en passant ‘color’ à console_putchar_at_attr
    uint8_t hours, minutes, seconds;
    ticks_to_time(ticks, &hours, &minutes, &seconds);
    
    // Positionner l'heure en haut à droite (colonne 66 sur 80)
    char time_str[12];
    snprintf(time_str, sizeof(time_str), " %02u:%02u:%02u ", hours, minutes, seconds);
    
    // Écrire chaque caractère à sa position
    int start_col = VGA_WIDTH - strlen(time_str);
    for (int i = 0; i < (int)strlen(time_str); i++) {
        console_putchar_at_attr(0, start_col + i, time_str[i], color);
    }
}

// ################################### RANDOM ################
// Génération aléatoire
static uint32_t lcg_seed = 1;

// Initialise le générateur
void srand(uint32_t seed) {
    lcg_seed = seed;
}

// Retourne un pseudo-aléatoire [0, RAND_MAX]
int rand(void) {
    // Paramètres glibc-like
    lcg_seed = lcg_seed * 1103515245 + 12345;
    return (int)((lcg_seed >> 16) & 0x7FFF);
}
