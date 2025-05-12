//
// Created by cash31 on 03/04/2025.
//

#ifndef TIMER_H
#define TIMER_H

#include <inttypes.h>
#include <stdbool.h>

typedef struct { uint8_t sec, min, hour, day, month, year; } rtc_time_t;

// Identifiant de l'interruption timer
#define IRQ_TIMER 32 // 32 (vecteur pour IRQ0)

#define FREQUENCE_SYSTEM 1193180
#define TIMER_FREQUENCY 1000

void init_timer();

void timer_handler_c();

void ticks_to_time(uint32_t ticks, uint8_t *hours, uint8_t *minutes, uint8_t *seconds);

void display_uptime();

bool rtc_get_datetime(rtc_time_t *dt);

void display_date();

void display_uptime_boxed();

void display_uptime_animated();


// Génération aléatoires des numéros
void srand(uint32_t seed);
int  rand(void);


#endif //TIMER_H
