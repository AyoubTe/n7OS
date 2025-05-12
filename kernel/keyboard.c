#include <n7OS/keyboard.h>
#include <n7OS/cpu.h>
#include <n7OS/irq.h>
#include <stdbool.h>

// Buffer circulaire pour stocker les caractères
static char keyboard_buffer[BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

// Flag pour indiquer qu'on a reçu un préfixe 0xE0
static bool ext_scancode = false;

// Table de conversion scancode -> ASCII (QWERTY)
static const char ascii_table[128] = {
    0,    // 0x00
    27,   // 0x01 - ESC
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t',  // 0x0F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,    // 0x1D - LEFT CTRL
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    // 0x2A - LEFT SHIFT
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,    // 0x36 - RIGHT SHIFT
    '*',  // 0x37
    0,    // 0x38 - ALT
    ' ',  // 0x39 - SPACE
};

char scancode_to_ascii(uint8_t scancode) {
    // Ignore les codes de relâchement de touche
    if (scancode & 0x80) {
        return 0;
    }
    
    // Retourne le caractère correspondant au scancode
    if (scancode < sizeof(ascii_table)) {
        return ascii_table[scancode];
    }
    
    return 0;
}

void keyboard_interrupt(void) {
    uint8_t status = inb(0x64);
    if (status & 0x01) {
        uint8_t sc = inb(KEYBOARD_PORT);
        if (sc == 0xE0) {
            ext_scancode = true;
        } else {
            char c;
            if (ext_scancode) {
                c = (char)(sc | 0x80);
                ext_scancode = false;
            } else {
                c = scancode_to_ascii(sc);
            }
            if (c) {
                int next = (buffer_head + 1) % BUFFER_SIZE;
                if (next != buffer_tail) {
                    keyboard_buffer[buffer_head] = c;
                    buffer_head = next;
                }
            }
        }
    }
    outb(0x20, 0x20);
}


// void keyboard_interrupt(void) {
//     // Acquittement partiel : lire le status
//     uint8_t status = inb(0x64);

//     // Vérifier qu'il y a bien un scancode disponible
//     if (status & 0x01) {
//         uint8_t sc = inb(KEYBOARD_PORT);

//         char c;
//         if (sc == 0xE0) {
//             // Préfixe pour scancodes étendus
//             ext_scancode = true;
//             goto done;
//         }

//         if (ext_scancode) {
//             // vrai scancode étendu : transmettre brut, bit7=1 pour distinguer
//             c = (char)(sc | 0x80);
//             ext_scancode = false;
//         } else {
//             // scancode normal → ASCII
//             c = scancode_to_ascii(sc);
//         }

//         // Si on a un caractère valide (0 si pas imprimable et pas étendu)
//         if (c) {
//             int next = (buffer_head + 1) % BUFFER_SIZE;
//             if (next != buffer_tail) {
//                 keyboard_buffer[buffer_head] = c;
//                 buffer_head = next;
//             }
//         }
//     }

// done:
//     // Acquitter l'interruption au PIC
//     outb(0x20, 0x20);
// }


// Lecture bloquonte
char kgetch(void) {
    char c;
    
    // Désactive les interruptions pendant la lecture
    cli();
    
    // Attend qu'un caractère soit disponible
    while (buffer_head == buffer_tail) {  // Changed from head to buffer_head
        // Réactive les interruptions pendant l'attente
        sti();
        hlt();
        cli();
    }
    
    // Lit le caractère
    c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    
    // Réactive les interruptions
    sti();
    
    return c;
}


/**
 * @brief Lit un caractère du buffer clavier, ou retourne -1 si vide.
 */
// une lecture non-bloquante
int kgetch_nb(void) {
    int c = -1;
    // Πas besoin de cli()/sti() ici : le buffer est mis à jour uniquement en IRQ
    if (buffer_head != buffer_tail) {
        c = keyboard_buffer[buffer_tail];
        buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    }
    return c;
}


void init_keyboard(void) {
    // Démasque l'IRQ1 (clavier)
    outb(inb(0x21) & ~(1 << 1), 0x21);
    
    // Installe le handler
    init_irq_entry(0x21, (uint32_t)keyboard_handler);
    
    // Vide les buffers
    buffer_head = buffer_tail = 0;
}