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

/**
 * Convertit un scancode en caractère ASCII.
 *
 * Cette fonction vérifie si le scancode correspond à une touche enfoncée
 * ou relâchée. Elle ignore les scancodes de relâchement (lorsque le bit 7 est défini)
 * et retourne le caractère correspondant en consultant une table de correspondance
 * (ascii_table). Si le scancode est supérieur ou égal à la taille de la table, la fonction
 * retourne 0.
 *
 * @param scancode Le code du scancode à convertir (valeur non signée de 8 bits).
 * @return Le caractère ASCII associé au scancode ou 0 si le scancode représente une touche de relâchement
 *         ou est invalide.
 */
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

/**
 * keyboard_interrupt - Handles keyboard interrupts.
 *
 * This function is invoked when a keyboard interrupt occurs. It performs the
 * following operations:
 *   1. Reads the keyboard controller status from port 0x64.
 *   2. If the status indicates that data is available (status & 0x01), it reads the
 *      scancode from the keyboard port.
 *   3. Checks if the scancode is 0xE0, which indicates the start of an extended
 *      scancode sequence. If so, it sets the 'ext_scancode' flag.
 *   4. If 'ext_scancode' is set, it processes the scancode as an extended key by
 *      setting its high bit; otherwise, it converts the scancode to an ASCII
 *      character using the 'scancode_to_ascii' function.
 *   5. If a valid ASCII character is derived and there is space in the keyboard
 *      buffer (circular queue managed by 'buffer_head' and 'buffer_tail'), the
 *      character is stored in the buffer.
 *   6. Finally, it sends an End Of Interrupt (EOI) signal to the keyboard controller 
 *      by writing to port 0x20.
 *
 * Note: The following globals are used in this function:
 *   - BUFFER_SIZE: the total size of the keyboard buffer.
 *   - buffer_head: the index of the next position to write in the keyboard buffer.
 *   - buffer_tail: the index marking the start of data in the keyboard buffer.
 *   - keyboard_buffer: the array that holds the buffered keyboard characters.
 *   - ext_scancode: a flag indicating that an extended scancode has been partially processed.
 */
void keyboard_interrupt() {
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

/**
 * @brief Lit de manière bloquante un caractère à partir du tampon clavier.
 *
 * Cette fonction lit un caractère en désactivant les interruptions pour garantir
 * une lecture cohérente et sécurisée. Elle attend en boucle tant qu'un caractère
 * n'est pas disponible dans le buffer, réactivant temporairement les interruptions
 * et mettant le processeur en veille (hlt) afin de réduire la consommation des ressources.
 * Une fois un caractère disponible, il est extrait du tampon et les interruptions sont
 * réactivées avant le retour de la fonction.
 *
 * @return char Le caractère lu provenant du tampon clavier.
 */
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


/**
 * init_keyboard - Initialise le clavier pour le système.
 *
 * Cette fonction effectue les opérations suivantes :
 * 1. Démasque l'IRQ1 (ligne d'interruption du clavier) en modifiant le registre de masque du PIC.
 * 2. Installe le gestionnaire d'interruption dédié au clavier via init_irq_entry.
 * 3. Réinitialise les index de tampon en vidant les buffers de saisie.
 *
 * Elle doit être appelée lors de l'initialisation du noyau pour permettre la gestion des entrées clavier.
 */
void init_keyboard(void) {
    // Démasque l'IRQ1 (clavier)
    outb(inb(0x21) & ~(1 << 1), 0x21);
    
    // Installe le handler
    init_irq_entry(0x21, (uint32_t)keyboard_handler);
    
    // Vide les buffers
    buffer_head = buffer_tail = 0;
}