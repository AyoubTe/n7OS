
//
// ⵉⵜⵡⵢⴰⵔⴰ ⵔⴷ ⴰⵢⵢⵓⴱ ⴰⵙⵙⴰⵎⵉ ⴳ 23/04/2025.
//

// ⵡⴰⵏⵏⴰ ⴰⴷ ⴻⵍⴳⵔⵉⵏ

#include <n7OS/console.h>
#include <n7OS/keyboard.h>
#include <n7OS/timer.h>
#include <n7OS/processus.h>
#include <string.h>
#include <stdio.h>

#define GAME_WIDTH   40
#define GAME_MARGIN  ((VGA_WIDTH - GAME_WIDTH) / 2)
#define FLOOR_Y      (GAME_MARGIN - 2)
#define JUMP_HEIGHT  4
#define OBSTACLE_GAP 30
#define SPEED_STEP   500
#define MAX_SPEED    5

static int       x_runner, y_runner, vx, vy, on_ground;
static int       ticks_cnt;
typedef struct { int x; } obstacle_t;
#define MAX_OBS 200
static obstacle_t obstacles[MAX_OBS];
static int        n_obs;

/**
 * draw_background - Efface et dessine le sol du jeu.
 *
 * Cette fonction réalise deux opérations successives :
 * 1. Efface la zone de jeu (à partir de la ligne 2 jusqu'à GAME_MARGIN), en remplissant chaque cellule
 *    par un espace (' ') avec l'attribut de couleur 0x0F.
 * 2. Dessine le sol en affichant un trait horizontal constitué du caractère '_' sur la ligne FLOOR_Y+1,
 *    décalé horizontalement de GAME_MARGIN, avec l'attribut de couleur 0x2E.
 *
 * Les dimensions et emplacements des éléments graphiques sont déterminés par les constantes GAME_MARGIN,
 * GAME_WIDTH et FLOOR_Y.
 *
 * @return void
 */
// Efface et dessine le sol
static void draw_background() {
    for (int y = 2; y < GAME_MARGIN; y++)
        for (int x = 0; x < GAME_WIDTH; x++)
            console_putchar_at_attr(y, x + GAME_MARGIN, ' ', 0x0F);
    for (int x = 0; x < GAME_WIDTH; x++)
        console_putchar_at_attr(FLOOR_Y+1, x + GAME_MARGIN, '_', 0x2E);
}

/**
 * draw_runner - Dessine le coureur en rouge sur la console.
 *
 * Cette fonction affiche le caractère 'A' en utilisant la couleur rouge 
 * (attribut 0x4C) sur la console. La position d'affichage du coureur est déterminée 
 * par la coordonnée y_runner pour l'ordonnée et x_runner décalé de GAME_MARGIN pour l'abscisse.
 */
// Dessine le coureur en rouge
static void draw_runner() {
    console_putchar_at_attr(y_runner, x_runner + GAME_MARGIN, 'A', 0x4C);
}

/**
 * draw_obstacles - Dessine les obstacles sur la console.
 *
 * Cette fonction parcourt tous les obstacles définis dans le tableau global "obstacles".
 * Pour chaque obstacle, elle calcule la position horizontale en soustrayant "ticks_cnt" de
 * la coordonnée "x" de l'obstacle. Si la position ajustée se situe dans les limites du jeu
 * (entre 0 et GAME_WIDTH), le caractère '#' est affiché sur la ligne spécifiée par "FLOOR_Y",
 * avec une couleur d'affichage magenta (attribut 0x5F) et décalé horizontalement par "GAME_MARGIN".
 *
 * Note :
 * - La variable "n_obs" détermine le nombre total d'obstacles à traiter.
 * - La fonction n'affiche l'obstacle que si sa position ajustée se trouve encore dans la région
 *   visible du jeu.
 */
// Dessine les obstacles en magenta
static void draw_obstacles() {
    for (int i = 0; i < n_obs; i++) {
        int x = obstacles[i].x - ticks_cnt;
        if (x >= 0 && x < GAME_WIDTH)
            console_putchar_at_attr(FLOOR_Y, x + GAME_MARGIN, '#', 0x5F);
    }
}

/**
 * update_physics - Met à jour la physique et génère des obstacles.
 *
 * Cette fonction effectue les opérations suivantes :
 *   - Ajoute un nouvel obstacle à la position GAME_WIDTH + ticks_cnt à intervalles réguliers (définis par OBSTACLE_GAP),
 *     tant que le nombre d'obstacles présents (n_obs) est inférieur à MAX_OBS.
 *   - Incrémente le compteur de ticks (ticks_cnt) en fonction de la vitesse horizontale (vx).
 *   - Augmente la vitesse horizontale (vx) à chaque SPEED_STEP, jusqu'à une limite maximale (MAX_SPEED).
 *   - Gère la physique verticale lorsque le personnage n'est pas au sol :
 *       * Incrémente la vitesse verticale (vy) pour simuler l'accélération due à la gravité.
 *       * Met à jour la position verticale du personnage (y_runner) en fonction de vy.
 *       * Corrige la position et réinitialise vy si le personnage atteint ou dépasse la position du sol (FLOOR_Y),
 *         en définissant également le flag on_ground pour indiquer que le personnage est au sol.
 */
// Mise à jour physique et génération d’obstacles
static void update_physics() {
    if (ticks_cnt % OBSTACLE_GAP == 0 && n_obs < MAX_OBS)
        obstacles[n_obs++].x = GAME_WIDTH + ticks_cnt;
    ticks_cnt += vx;
    if ((ticks_cnt % SPEED_STEP) == 0 && vx < MAX_SPEED)
        vx++;
    if (!on_ground) {
        vy++;
        y_runner += vy;
        if (y_runner >= FLOOR_Y) {
            y_runner  = FLOOR_Y;
            vy        = 0;
            on_ground = 1;
        }
    }
}

/**
 * Vérifie s'il y a collision entre le runner et un obstacle.
 *
 * La fonction parcourt la liste des obstacles et ajuste leur position horizontale en
 * soustrayant le compteur de ticks (ticks_cnt) à leur coordonnée x. Si, après ajustement,
 * la position de l'obstacle correspond à x_runner et que le runner est au niveau du sol (FLOOR_Y),
 * une collision est détectée.
 *
 * @return 1 si une collision est détectée, 0 sinon.
 */
// Vérifie collision
static int check_collision() {
    for (int i = 0; i < n_obs; i++) {
        int x = obstacles[i].x - ticks_cnt;
        if (x == x_runner && y_runner == FLOOR_Y)
            return 1;
    }
    return 0;
}

void runner_loop(void) {
    char buf[32];
    int score;
    int choice;

    do {
        // Initialisation du jeu
        console_putbytes("\f", 1);
        printf("=== Runner Game ===\n");
        printf("Press SPACE to jump and Q to quit.\n");

        x_runner   = 5;
        y_runner   = FLOOR_Y;
        vx         = 1;
        vy         = 0;
        on_ground  = 1;
        ticks_cnt  = 0;
        n_obs      = 0;

        update_cursor(0, 5);

        // Boucle principale
        while (1) {
            int c = kgetch_nb();
            if ((c == ' ' || c == 'z') && on_ground) {
                on_ground = 0;
                vy        = -JUMP_HEIGHT;
            } else if (c == 'q' || c == 'Q') {
                // Abandon volontaire
                goto game_end;
            }

            draw_background();
            update_physics();
            draw_obstacles();
            draw_runner();

            // Affiche la vitesse
            snprintf(buf, sizeof(buf), "V:%d ", vx);
            console_putchar_at_attr(2, GAME_MARGIN, buf[2], 0x3F);

            if (check_collision())
                break;

            schedule();
        }

    game_end:
        // Calcul et affichage du score
        score = ticks_cnt;
        snprintf(buf, sizeof(buf), "\nGame Over! Score: %d\n", score);
        console_putbytes(buf, strlen(buf));

        // Choix rejouer ou quitter
        console_putbytes("Play again? (y/n): ", 20);
        // Lecture bloquante d’un choix
        do {
            choice = kgetch_nb();
            schedule();
        } while (choice < 0);
        //console_putchar(choice);

    } while (! (choice == 'n' || choice == 'N'));
    
    // Vide l'écran avant de revenir en shell
    console_putbytes("\f", 1);

    // Une seule fois sorti de la boucle : retour au shell
    exit_p();
}

