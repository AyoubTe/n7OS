#include <n7OS/console.h>
#include <n7OS/keyboard.h>
#include <n7OS/timer.h>
#include <n7OS/processus.h>
#include <string.h>
#include <stdio.h>

#define GAME_WIDTH  40
#define GAME_MARGIN ((VGA_WIDTH - GAME_WIDTH) / 2)


#define FLOOR_Y    (GAME_MARGIN - 2)
#define JUMP_HEIGHT 4
#define OBSTACLE_GAP 30  // distance minimal entre obstacles

static int       x_runner = 5;
static int       y_runner = FLOOR_Y;
static int       vx = 1;    // vitesse d’avancement
static int       vy = 0;    // vitesse verticale
static int       on_ground = 1;
static int       tick_count = 0;

typedef struct { int x; } obstacle_t;
#define MAX_OBS  16
static obstacle_t obstacles[MAX_OBS];
static int        n_obs = 0;

// Dessine le fond et le sol
static void draw_background() {
    // effacer zone de jeu
    for (int y = 2; y < GAME_MARGIN; y++)
        for (int x = 0; x < GAME_WIDTH; x++)
            console_putchar_at(y, x, ' ');
    // le sol
    for (int x = 0; x < GAME_WIDTH; x++)
        console_putchar_at(FLOOR_Y+1, x, '_');
}

// Place le joueur
static void draw_runner() {
    console_putchar_at(y_runner, x_runner, 'A'); // A = avatar
}

// Place les obstacles
static void draw_obstacles() {
    for (int i = 0; i < n_obs; i++) {
        int x = obstacles[i].x - tick_count;
        if (x >= 0 && x < GAME_WIDTH)
            console_putchar_at(FLOOR_Y, x, '#');
    }
}

// Met à jour la physique
static void update_physics() {
    // générer un obstacle tous les OBSTACLE_GAP ticks
    if (tick_count % OBSTACLE_GAP == 0 && n_obs < MAX_OBS) {
        obstacles[n_obs++].x = GAME_WIDTH + tick_count;
    }
    // avancer
    tick_count += vx;

    // traitement de la chute
    if (!on_ground) {
        vy += 1; // gravité
        y_runner += vy;
        if (y_runner >= FLOOR_Y) {
            y_runner = FLOOR_Y;
            vy = 0;
            on_ground = 1;
        }
    }
}

// Gère la collision
static int check_collision() {
    for (int i = 0; i < n_obs; i++) {
        int x = obstacles[i].x - tick_count;
        if (x == x_runner && y_runner == FLOOR_Y)
            return 1;
    }
    return 0;
}

void runner_loop() {
    // Titre
    printf("=== Runner Game ===\n");
    printf("Player: ");
    printf("Ayoub");
    printf("\nPress space to jump.\n");

    // repositionne curseur en haut du jeu
    update_cursor(0, 4);

    // boucle de jeu
    while (1) {
        // entrée
        int c = kgetch_nb();
        if (c == ' ') {
            if (on_ground) {
                on_ground = 0;
                vy = -JUMP_HEIGHT;
            }
        } else if (c == 'q' || c == 'Q') {
            break; // quitter
        }

        // physique et dessin
        draw_background();
        update_physics();
        draw_obstacles();
        draw_runner();

        // collision ?
        if (check_collision()) {
            console_putbytes("\nGame Over!\n", 12);
            break;
        }

        // Synchronisation simple : céder la main
        schedule();
    }

    // Fin du jeu : retourne au shell
    exit_p();
}
