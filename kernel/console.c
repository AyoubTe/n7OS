#include <n7OS/console.h>
#include <n7OS/cpu.h>
#include <n7OS/timer.h>
#include <stddef.h>
#include <stdio.h>


/**
 * @file console.c
 * @brief Gestion de la console pour le noyau.
 */

/**
 * @var scr_tab
 * @brief Pointeur vers le tampon écran.
 * 
 * Cette variable pointe vers l'adresse de début du tampon écran, qui est utilisé
 * pour manipuler directement le contenu affiché à l'écran.
 */

/**
 * @var cursor_x
 * @brief Position horizontale du curseur.
 * 
 * Cette variable statique garde la trace de la position horizontale actuelle (colonne) 
 * du curseur à l'écran.
 */

/**
 * @var cursor_y
 * @brief Position verticale du curseur.
 * 
 * Cette variable statique garde la trace de la position verticale actuelle (ligne) 
 * du curseur à l'écran.
 */
uint16_t *scr_tab = (uint16_t *) SCREEN_ADDR;
static size_t cursor_x = 2;
static size_t cursor_y = 0;

/**
 * @brief Initialise la console en effaçant l'écran et en positionnant le curseur.
 *
 * Cette fonction remplit le tampon écran avec des espaces (' ') et définit leur attribut de couleur
 * sur blanc sur noir (0x0F). Ensuite, elle place le curseur dans le coin supérieur gauche de l'écran.
 *
 * @note Cette fonction suppose que le tampon écran (scr_tab) et les dimensions VGA 
 * (VGA_WIDTH et VGA_HEIGHT) sont définis et correctement initialisés dans console.h.
 */
void init_console() {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            scr_tab[y * VGA_WIDTH + x] = (0x0F << 8) | ' ';
        }
    }

    // Afficher le nom de l'OS sur la première ligne (à gauche)
    display_os_name ();

    printf("\n");
    printf("\n");

    init_timer();

    update_cursor(2, 0);
}

/**
 * @brief Met à jour la position du curseur à l'écran.
 *
 * Cette fonction place le curseur aux coordonnées (x, y) spécifiées
 * en envoyant les commandes appropriées au contrôleur VGA.
 *
 * @param x Coordonnée x (colonne) de la nouvelle position du curseur.
 * @param y Coordonnée y (ligne) de la nouvelle position du curseur.
 */
void update_cursor(size_t x, size_t y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x0F, PORT_CMD);  // Envoi de la commande 0x0F au port 0x3D4
    outb((uint8_t)(pos & 0xFF), PORT_DATA);
    outb(0x0E, PORT_CMD);  // Envoi de la commande 0x0E au port 0x3D4
    outb((uint8_t)((pos >> 8) & 0xFF), PORT_DATA);
}

/**
 * @brief Fait défiler l'écran d'une ligne vers le haut si le curseur dépasse la hauteur de l'écran.
 *
 * Cette fonction vérifie si la coordonnée y du curseur est supérieure ou égale à la hauteur de l'écran VGA.
 * Si c'est le cas, elle fait défiler l'écran d'une ligne vers le haut, 
 * supprimant ainsi la ligne supérieure et déplaçant toutes les autres lignes vers le haut.
 * La dernière ligne de l'écran est ensuite effacée.
 *
 * La fonction met à jour la position du curseur au début de la dernière ligne après le défilement.
 *
 * @note Cette fonction suppose que `cursor_y`, `cursor_x`, `VGA_HEIGHT`, `VGA_WIDTH` 
 * et `scr_tab` sont définis et correctement initialisés.
 */
void scroll_screen() {
    if (cursor_y >= VGA_HEIGHT) {
        for (size_t y = 3; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                scr_tab[(y - 1) * VGA_WIDTH + x] = scr_tab[y * VGA_WIDTH + x];
            }
        }
        
        // Effacer la dernière ligne
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            scr_tab[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (0x0F << 8) | ' ';
        }

        // On met le cursur dans la derbière ligne.
        cursor_y = VGA_HEIGHT - 1;
        cursor_x = 0;
    }
}

/**
 * @brief Écrit un caractère dans la console à la position actuelle du curseur.
 *
 * Cette fonction gère plusieurs caractères de contrôle :
 * - Caractères imprimables (ASCII 32 à 126) : affiche le caractère et avance le curseur.
 * - '\n' : déplace le curseur au début de la ligne suivante.
 * - '\b' : recule le curseur d'une position en effaçant le caractère à cette position.
 * - '\t' : avance le curseur jusqu'à la prochaine tabulation (tous les 8 colonnes).
 * - '\f' : efface l'écran et réinitialise la position du curseur.
 * - '\r' : ramène le curseur au début de la ligne courante.
 *
 * Si le curseur atteint la fin de l'écran, l'écran défile vers le haut.
 * La position du curseur est mise à jour après le traitement du caractère.
 *
 * @param c Le caractère à écrire sur la console.
 */
void console_putchar(char c) {
    if (c >= 32 && c < 127) {  // Caractères imprimables
        scr_tab[cursor_y * VGA_WIDTH + cursor_x] = (0x0F << 8) | c;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    } else if (c == '\n') {  // Nouvelle ligne
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {  // Retour arrière
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
        }
        scr_tab[cursor_y * VGA_WIDTH + cursor_x] = (0x0F << 8) | ' ';
    } else if (c == '\t') {  // Tabulation (remplie avec des espaces jusqu'à la prochaine colonne multiple de 8)
        do {
            scr_tab[cursor_y * VGA_WIDTH + cursor_x] = (0x0F << 8) | ' ';
            cursor_x++;
        } while (cursor_x % 8 != 0 && cursor_x < VGA_WIDTH);
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }

    } else if (c == '\f') {  // Effacer l'écran
        for (size_t y = 2; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                scr_tab[y * VGA_WIDTH + x] = (0x0F << 8) | ' ';
            }
        }

        // On place le curseur au début de la 3ème ligne (row=2, col=0)
        cursor_x = 0;
        cursor_y = 2;
 
    } else if (c == '\r') {  // Retour au début de la ligne
        cursor_x = 0;
    }

    scroll_screen();

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        cursor_y = VGA_HEIGHT - 1;
    }

    update_cursor(cursor_x, cursor_y);
}

/**
 * @brief Affiche une séquence de caractères sur la console.
 *
 * Cette fonction écrit un nombre spécifié de caractères à partir d'une chaîne donnée
 * sur la console, en appelant la fonction console_putchar pour chaque caractère.
 *
 * @param s La chaîne contenant les caractères à écrire sur la console.
 * @param len Le nombre de caractères à écrire à partir de la chaîne.
 */
void console_putbytes(const char *s, int len) {
    for (int i = 0; i < len; i++) {
        console_putchar(s[i]);
    }
}


/**
 * @brief a character to the console at a specific row and column.
 *
 * This function calculates the appropriate position in video memory using 
 * the formula: pos = row * VGA_WIDTH + col. It then writes the provided 
 * character, combined with a pre-defined color (CHAR_COLOR), directly into 
 * the video memory at that position.
 *
 * Parameters:
 *   @param row - The row index (0-indexed) where the character should be displayed.
 *   @param col - The column index (0-indexed) where the character should be displayed.
 *   @param c   - The character to display at the specified position.
 *
 * Note:
 *   - The video memory starts at the address defined by SCREEN_ADDR.
 *   - VGA_WIDTH specifies the width of the display in characters.
 */
void console_putchar_at(uint8_t row, uint8_t col, char c) {
    uint16_t *screen = (uint16_t*) SCREEN_ADDR;
    uint16_t pos = row * VGA_WIDTH + col;
    screen[pos] = (CHAR_COLOR << 8) | c; // Couleur et caractère
}


// #################### Les améliorartions ############################

/**
 * @brief Met un caractère coloré à l’écran.
 * @param row Ligne (0-index).
 * @param col Colonne (0-index).
 * @param c   Caractère ASCII.
 * @param attr Octet attribut (FG+BG+blink).
 */
void console_putchar_at_attr(uint8_t row, uint8_t col, char c, uint8_t attr) {
    uint16_t *screen = (uint16_t*) SCREEN_ADDR;
    uint16_t pos = row * VGA_WIDTH + col;
    screen[pos] = (attr << 8) | c;
}


// Afficher le nom de l'OS sur la première ligne (à gauche)
void display_os_name () {
    const char *os_name = "N7 OS";
    for (int i = 0; os_name[i] != '\0'; i++) {
        console_putchar_at(0, i, os_name[i]); // Fonction personnalisée
    }
}
