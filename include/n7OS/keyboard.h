#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <inttypes.h>

#define KEYBOARD_IRQ     1       // IRQ 1
#define KEYBOARD_PORT    0x60    // Port d'entrée du clavier
#define BUFFER_SIZE      256     // Taille du buffer circulaire

// Initialisation du clavier
void init_keyboard(void);

// Lecture d'un caractère (bloquant)
char kgetch(void);

// Lecture non bloquante
int kgetch_nb(void);

// Conversion scancode vers ASCII
char scancode_to_ascii(uint8_t scancode);

extern void keyboard_handler(void);

extern void keyboard_interrupt(void);


#endif