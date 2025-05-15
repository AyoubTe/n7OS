#include <n7OS/console.h>
#include <n7OS/processus.h>
#include <n7OS/keyboard.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_LINE 80
#define HISTORY_SIZE 10

extern void runner_loop(void);
extern void snake_loop(void);


// Type pour définer les apps
typedef int (*app_fn_t)(void);


// La fonction lannce_runner :
int lance_runner() {
    // Crée un processus « runner »
    pid_t pid = creer_p("runner", runner_loop);
    process_table[pid].priority = 100;  // priorité max
    if ((int)pid < 0) {
        printf("Impossible de demarrer le jeu runner\n");
        return -1;
    }
    return 0;
}

// La fonction qui lance le runner
int lance_snake(void) {
    pid_t pid = creer_p("snake", snake_loop);
    if ((int)pid < 0) {
        printf("Impossible de demarrer Snake\n");
        return -1;
    }
    process_table[pid].priority = 100; // priorité max
    return 0;
}

// static struct {
//     const char *name;
//     app_fn_t    fn;
// } apps[] = {
//     { "runner", lance_runner },
//     { "snake",  lance_snake  },
//     { NULL, NULL}
// };


void shell_loop() {
    char line[MAX_LINE+1];
    int len = 0;

    // Historique
    char history[HISTORY_SIZE][MAX_LINE+1] = {0};
    int history_index = 0;

    printf("\n$ ");
    while (1) {
        int c = kgetch_nb();
        if (c < 0) {
            schedule();
            continue;
        }
        
        if (c == (char)0xE0) {
            // Scancode étendu, lire suivant
            c = kgetch_nb();

            uint8_t sc = c & 0x7F;

            if (sc == 0x48) { // flèche ↑ 0x48
                if (history_index > 0) {
                    history_index--;
                    // Effacer ligne actuelle
                    while (len > 0) {
                        console_putchar('\b');
                        len--;
                    }
                    // Charger la dernière commande
                    strcpy(line, history[history_index]);
                    printf("%s", line);
                    len = strlen(line);
                }
                continue;
            }
        }

        if (c == '\r' || c == '\n') {
            line[len] = '\0';
            console_putchar('\n');

            // Sauvegarde dans l’historique
            if (len > 0 && (history_index == 0 || strcmp(line, history[history_index - 1]) != 0)) {
                if (history_index < HISTORY_SIZE) {
                    strcpy(history[history_index++], line);
                }
            }

            if (len > 0) {
                char *cmd = strtok(line, " ");
                char *arg = strtok(NULL, " ");

                if (strcmp(cmd, "help") == 0) {
                    printf("Commandes disponibles:\n");
                    printf("  help       : liste des commandes\n");
                    printf("  clear      : efface l’écran\n");
                    printf("  run <app>  : lance une application\n");
                    printf("  apps       : liste les apps disponibles\n");
                    printf("  poweroff   : éteint le système\n");

                } else if (strcmp(cmd, "clear") == 0) {
                    console_putbytes("\f", 1);

                } else if (strcmp(cmd, "apps") == 0) {
                    printf("Applications disponibles:\n");
                    printf("  - snake       : jeu du serpant\n");
                    printf("  - runner      : jeu de course\n");

                } else if (strcmp(cmd, "run") == 0) {
                    if (!arg) {
                        printf("Usage : run <nom_app>\n");
                    } else {
                        // int found = 0;
                        // for (int i = 0; apps[i].name; i++) {
                        //     if (strcmp(arg, apps[i].name) == 0) {
                        //         apps[i].fn();
                        //         found = 1;
                        //         break;
                        //     }
                        // }
                        // if (!found) {
                        //     printf("App inconnue: %s\n", arg);
                        // }
                        if (strcmp(arg, "runner") == 0) {
                            lance_runner();
                        }
                        else if (strcmp(arg, "snake") == 0) {
                            lance_snake();
                        }
                        else {
                            printf("App inconnue: %s\n", arg);
                        }
                    }

                } else if (strcmp(cmd, "poweroff") == 0) {
                    shutdown(1);

                } else {
                    printf("Commande inconnue: %s\n", cmd);
                }
            }

            len = 0;
            printf("$ ");
        }
        else if (c == '\b' && len > 0) {
            len--;
            console_putchar('\b');

        }
        else if (c >= 32 && c < 127 && len < MAX_LINE) {
            line[len++] = c;
            console_putchar(c);
        }
    }
}