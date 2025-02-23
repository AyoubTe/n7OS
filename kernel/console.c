#include <n7OS/console.h>
#include <stddef.h>


uint16_t *scr_tab;

static size_t cursor_x = 0;
static size_t cursor_y = 0;
static uint8_t color;

void init_console() {
    scr_tab= (uint16_t *) SCREEN_ADDR;
    
}

void console_putchar(const char c) {
    if (c == 'a') 
        scr_tab[0]= CHAR_COLOR<<8|c;
}

void console_putbytes(const char *s, int len) {
    for (int i= 0; i<len; i++) {
        console_putchar(s[i]);
    }
}