
//
// Created by ⴰⵢⵢⵓⴱ ⴰⵙⵙⴰⵎⵉ on 29/04/2025.
//

#include <n7OS/console.h>
#include <n7OS/keyboard.h>
#include <n7OS/processus.h>
#include <n7OS/timer.h>
#include <n7OS/irq.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

// Dimensions et position du terrain
#define W            VGA_WIDTH
#define H            VGA_HEIGHT
#define UI_LINES     6
#define GAME_HEIGHT  (H - UI_LINES - 2)
#define GAME_TOP     5
#define GAME_BOTTOM  (H - UI_LINES + 3)
#define GAME_WIDTH   40
#define GAME_LEFT    ((W - GAME_WIDTH) / 2)
#define GAME_RIGHT   (GAME_LEFT + GAME_WIDTH - 1)

// Longueur initiale et maximale, vies
#define INIT_LEN     1             // longueuran initiale réduite à 1
#define MAX_LEN      (GAME_WIDTH * GAME_HEIGHT)
#define MAX_LIVES    5
#define HEART_CODE   3             // code CP437 pour ♥

#define DELAY_TICKS  200            // nombre de ticks entre chaque mise à jour

// Attributs VGA (F = foreground, B = background)
#define C_BLUE_BG    0x1F  // blanc sur bleu  
#define C_RED_FG     0x4F  // rouge sur noir  
#define C_GREEN_FG   0x2F  // vert sur noir  
#define C_HEART_ATTR 0x4C  // rouge sur noir (cœur)

typedef struct { int x, y; } coord;

// État du jeu
static coord snake[MAX_LEN];
static int   length, dx, dy;
static coord food;
static bool  alive;
static int   lives;

// Externe du timer.c
extern volatile uint32_t ticks;

/**
 * @brief Initialise la graine du générateur de nombres aléatoires.
 *
 * Cette fonction configure la graine utilisée par le générateur de nombres aléatoires
 * en récupérant l'heure courante via le RTC (CMOS). Elle récupère les secondes, minutes
 * et heures actuelles pour calculer une valeur de graine, qui est ensuite passée à
 * srand() afin d'initialiser le générateur.
 *
 * @note Aucune valeur n'est retournée par cette fonction.
 */
// Initialisation de la graine aléatoire via CMOS
static void init_random_seed(void) {
    rtc_time_t now;
    rtc_get_datetime(&now);
    uint32_t seed = now.sec + now.min*60 + now.hour*3600;
    srand(seed);
}

/**
 * @brief Place la nourriture dans le cadre de jeu.
 *
 * Cette fonction génère de manière aléatoire une position pour la nourriture,
 * en veillant à ce qu'elle soit placée à l'intérieur des limites du cadre de
 * jeu défini par GAME_LEFT, GAME_TOP, GAME_WIDTH et GAME_HEIGHT. La position
 * calculée est ensuite stockée dans la structure 'food' et affichée sur la console
 * sous la forme d'un caractère '@' avec l'attribut de couleur C_GREEN_FG.
 *
 * @note La fonction utilise rand() pour la génération aléatoire et console_putchar_at_attr()
 *       pour afficher la nourriture à l'écran.
 */
// Place la nourriture en vert à l’intérieur du cadre
static void place_food(void) {
    food.x = GAME_LEFT + 1 + (rand() % (GAME_WIDTH - 2));
    food.y = GAME_TOP  + 1 + (rand() % (GAME_HEIGHT - 1));
    console_putchar_at_attr(food.y, food.x, '@', C_GREEN_FG);
}

/**
 * draw_border - Dessine un cadre bleu autour du terrain de jeu.
 *
 * Cette fonction trace un contour sur l'interface graphique du jeu en utilisant
 * des caractères spécifiques à chaque bord. Les bords supérieur et inférieur
 * sont dessinés avec le caractère '-', tandis que les bords gauche et droit sont
 * dessinés avec le caractère '|'. Les quatre coins du cadre sont représentés
 * par le caractère '+'.
 *
 * Les attributs de couleur sont définis par la constante C_BLUE_BG pour donner
 * un fond bleu au cadre.
 *
 * Utilise les constantes suivantes pour délimiter le terrain de jeu :
 *   - GAME_LEFT : position de la colonne de gauche.
 *   - GAME_RIGHT : position de la colonne de droite.
 *   - GAME_TOP : position de la ligne supérieure.
 *   - GAME_BOTTOM : position de la ligne inférieure.
 *
 * La fonction utilise la routine console_putchar_at_attr() pour afficher chaque 
 * caractère à la position donnée avec l'attribut de couleur spécifié.
 */
// Dessine un cadre bleu autour du terrain
static void draw_border(void) {
    for (int x = GAME_LEFT; x <= GAME_RIGHT; x++) {
        console_putchar_at_attr(GAME_TOP,    x, '-', C_BLUE_BG);
        console_putchar_at_attr(GAME_BOTTOM, x, '-', C_BLUE_BG);
    }
    for (int y = GAME_TOP; y <= GAME_BOTTOM; y++) {
        console_putchar_at_attr(y, GAME_LEFT,  '|', C_BLUE_BG);
        console_putchar_at_attr(y, GAME_RIGHT, '|', C_BLUE_BG);
    }
    console_putchar_at_attr(GAME_TOP,    GAME_LEFT,  '+', C_BLUE_BG);
    console_putchar_at_attr(GAME_TOP,    GAME_RIGHT, '+', C_BLUE_BG);
    console_putchar_at_attr(GAME_BOTTOM, GAME_LEFT,  '+', C_BLUE_BG);
    console_putchar_at_attr(GAME_BOTTOM, GAME_RIGHT, '+', C_BLUE_BG);
}

/**
 * draw_lives - Affiche les vies du joueur sous forme de cœurs rouges.
 *
 * Cette fonction affiche les vies du joueur en utilisant des cœurs rouges positionnés
 * juste au-dessus du cadre de jeu. Pour chaque vie active (jusqu'à 'lives'), elle dessine
 * un cœur à l'endroit calculé par rapport aux positions GAME_TOP et GAME_LEFT. Si le nombre
 * de vies est inférieur à MAX_LIVES, les positions restantes sont effacées (remplies d'espaces)
 * pour s'assurer que les anciens cœurs ne restent pas visibles.
 *
 * Variables Globales et Constantes utilisées :
 *   - lives         : Entier représentant le nombre actuel de vies du joueur.
 *   - MAX_LIVES     : Nombre maximum de vies possible.
 *   - GAME_TOP      : Position verticale de référence dans la console pour le dessin.
 *   - GAME_LEFT     : Position horizontale de référence dans la console pour le dessin.
 *   - HEART_CODE    : Code du caractère à afficher pour représenter un cœur.
 *   - C_HEART_ATTR  : Attribut de couleur appliqué au caractère de cœur.
 *
 * Remarque :
 *   Il est important que toutes les variables et constantes utilisées soient correctement
 *   initialisées avant l'appel à cette fonction pour garantir un comportement correct du dessin.
 */
// Affiche les vies en cœurs rouges juste au-dessus du cadre
static void draw_lives(void) {
    for (int i = 0; i < lives; i++) {
        console_putchar_at_attr(GAME_TOP - 1, GAME_LEFT + i * 2, HEART_CODE, C_HEART_ATTR);
    }
    for (int i = lives; i < MAX_LIVES; i++) {
        console_putchar_at(GAME_TOP - 1, GAME_LEFT + i * 2, ' ');
    }
}

/**
 * @brief Efface l'intérieur de la zone de jeu.
 *
 * Cette fonction vide la zone intérieure délimitée par le cadre du jeu, 
 * c'est-à-dire l'espace situé entre GAME_TOP et GAME_BOTTOM ainsi qu'entre
 * GAME_LEFT et GAME_RIGHT. Chaque cellule de cette zone est remplacée par 
 * un caractère espace afin de "nettoyer" l'affichage.
 */
// Efface l’intérieur du jeu (zone sous le cadre)
static void clear_game_area(void) {
    for (int y = GAME_TOP + 1; y < GAME_BOTTOM; y++) {
        for (int x = GAME_LEFT + 1; x < GAME_RIGHT; x++) {
            console_putchar_at(y, x, ' ');
        }
    }
}

/**
 * Dessine la tête du serpent en rouge et efface l'ancienne position de la queue.
 *
 * La fonction récupère la coordonnée de la queue du serpent (position à l'index 'length')
 * et affiche un caractère espace à cet endroit pour l'effacer de l'affichage.
 * Ensuite, elle dessine un caractère 'O' en rouge à la position de la tête du serpent
 * (index 0 dans le tableau 'snake').
 *
 * Dépendances :
 * - Utilise les variables globales 'snake' (tableau de coordonnées) et 'length' (nombre d'éléments).
 * - Utilise les fonctions 'console_putchar_at' et 'console_putchar_at_attr' pour l'affichage.
 */
// Dessine la tête en rouge et efface la queue
static void draw_snake(void) {
    coord tail = snake[length];
    console_putchar_at(tail.y, tail.x, ' ');
    console_putchar_at_attr(snake[0].y, snake[0].x, 'O', C_RED_FG);
}

/**
 * move_snake - Déplace le serpent et gère les collisions avec le cadre, le corps et la nourriture.
 *
 * Cette fonction déplace le serpent en décalant chaque segment vers la position de son prédécesseur,
 * puis met à jour la position de la tête en ajoutant les valeurs de déplacement dx et dy.
 * 
 * Elle vérifie ensuite si la tête du serpent entre en collision avec le cadre du jeu ou avec un segment de son propre corps.
 * En cas de collision avec le cadre ou le corps, la fonction retourne false, indiquant une fin de partie.
 *
 * Si la tête rencontre la nourriture, la longueur du serpent est augmentée (tant que celle-ci n'atteint pas la
 * valeur maximale définie par MAX_LEN - 1) et une nouvelle position pour la nourriture est générée.
 *
 * Retourne:
 *   true  - si le déplacement s'est effectué sans collision fatale.
 *   false - si une collision fatale est détectée.
 */
// Déplace le serpent, gère collisions et nourriture
static bool move_snake(void) {
    for (int i = length; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0].x += dx;
    snake[0].y += dy;

    // Collision avec le cadre
    if (snake[0].x <= GAME_LEFT || snake[0].x >= GAME_RIGHT ||
        snake[0].y <= GAME_TOP  || snake[0].y >= GAME_BOTTOM)
        return false;

    // Collision avec le corps
    for (int i = 1; i < length; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
            return false;
    }

    // Nourriture ?
    if (snake[0].x == food.x && snake[0].y == food.y) {
        if (length < MAX_LEN - 1) length++;
        place_food();
    }
    return true;
}


void snake_loop(void) {
    console_putbytes("\f", 1);

    init_random_seed();
    length = INIT_LEN;
    dx = 1; dy = 0;
    alive = true;
    lives = 3;

    clear_game_area();
    draw_border();
    draw_lives();
    place_food();

    for (int i = 0; i < length; i++) {
        snake[i].x = GAME_LEFT + GAME_WIDTH / 2 - i;
        snake[i].y = GAME_BOTTOM - 1;
        console_putchar_at_attr(snake[i].y, snake[i].x, 'O', C_RED_FG);
    }

    const char *msg = "Snake: flèches pour diriger, Q pour quitter.";
    int col = (W - strlen(msg)) / 2;
    update_cursor(col, H - 1);
    printf("%s", msg);

    uint32_t last_tick = ticks;

    while (alive) {
        int c = kgetch_nb();
        if (c == (char)0xE0) {
            c = kgetch_nb();  // lire la vraie flèche
        }

        if (c == 'q' || c == 'Q') {
            alive = false;
            break;
        }

        uint8_t sc = c & 0x7F;

        // Empêche le demi-tour immédiat
        if (sc == 0x4B && dx != 1) { dx = -1; dy = 0; } // ←
        if (sc == 0x4D && dx != -1) { dx = 1; dy = 0; } // →
        if (sc == 0x48 && dy != 1) { dx = 0; dy = -1; } // ↑
        if (sc == 0x50 && dy != -1) { dx = 0; dy = 1; } // ↓

        if (!move_snake()) {
            lives--;
            draw_lives();
            if (lives == 0) {
                alive = false;
                break;
            }
            // repositionner tête après collision
            snake[0].x = GAME_LEFT + GAME_WIDTH / 2;
            snake[0].y = GAME_BOTTOM - 1;
            dx = 1; dy = 0;
        }

        draw_snake();

        while ((ticks - last_tick) < DELAY_TICKS) {
            schedule();
        }
        last_tick = ticks;
    }

    update_cursor(0, H - 1);
    printf("\nGame Over! Score: %d\n", length - INIT_LEN);
    printf("Rejouer ? (O/N) ");

    char choice;
    do {
        choice = kgetch_nb();
        if (choice < 0) {
            schedule();
            continue;
        }
        console_putchar(choice);
    } while (choice != 'O' && choice != 'o' && choice != 'N' && choice != 'n');

    if (choice == 'O' || choice == 'o') {
        console_putbytes("\f", 1);
        snake_loop();
        return;
    }

    console_putbytes("\f", 1);
    exit_p();
}