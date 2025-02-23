#include <n7OS/console.h>
#include <n7OS/cpu.h>
#include <stddef.h>

uint16_t *scr_tab = (uint16_t *) SCREEN_ADDR;
static size_t cursor_x = 0;
static size_t cursor_y = 0;

void init_console() {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            scr_tab[y * VGA_WIDTH + x] = (0x0F << 8) | ' ';
        }
    }
    update_cursor(0, 0);
}

void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void scroll_screen() {
    if (cursor_y >= VGA_HEIGHT) {
        for (size_t y = 1; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                scr_tab[(y - 1) * VGA_WIDTH + x] = scr_tab[y * VGA_WIDTH + x];
            }
        }
        // Effacer la dernière ligne
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            scr_tab[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (0x0F << 8) | ' ';
        }
        cursor_y = VGA_HEIGHT - 1;
    }
}

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
    } else if (c == '\t') {  // Tabulation (alignement à 8)
        int next_tab_stop = (cursor_x + 8) & ~(8 - 1);
        if (next_tab_stop >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        } else {
            cursor_x = next_tab_stop;
        }
    } else if (c == '\f') {  // Effacer l'écran
        init_console();
        return;  // Pas besoin d'update_cursor après un clear
    } else if (c == '\r') {  // Retour au début de la ligne
        cursor_x = 0;
    }

    scroll_screen();
    update_cursor(cursor_x, cursor_y);
}

void console_putbytes(const char *s, int len) {
    for (int i = 0; i < len; i++) {
        console_putchar(s[i]);
    }
}
