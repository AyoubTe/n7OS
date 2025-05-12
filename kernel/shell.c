#include <n7OS/console.h>
#include <n7OS/processus.h>
#include <n7OS/keyboard.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_LINE 80

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
//     //{ "autre_app",     lance_autre_app },
//     // ajoute ici d'autres apps...
//     { NULL, NULL}
// };


void shell_loop() {
    char line[MAX_LINE+1];
    int len = 0;

    printf("\n$ ");                   // invite
    while (1) {
        int c = kgetch_nb();
        if (c < 0) {
            // rien à lire : on céde le CPU au timer / noyau
            schedule();
            continue;
        }
        // Écho et gestion des touches spéciales
        if (c == '\r' || c == '\n') {
            line[len] = '\0';
            console_putchar('\n');
            if (len > 0) {
                // simple parsing
                char *cmd = strtok(line, " ");
                char *arg = strtok(NULL, " ");
                if (strcmp(cmd, "help") == 0) {
                    printf("help: liste des commandes\n");
                    printf("clear: efface lcl'ecran\n");
                    printf("run <nom app>: lance l'app\n");
                    printf("poweroff: arrete le systeme\n");

                } else if (strcmp(cmd, "clear") == 0) {
                    console_putbytes("\f", 1);

                } else if (strcmp(cmd, "run") == 0) {
                    if (!arg) {
                        printf("Usage : run <nom_app>\n");
                    } else {
                        // // trim espaces et retours chariot
                        // char *p = arg + strlen(arg) - 1;
                        // while (p >= arg && (*p==' '||*p=='\r'||*p=='\n')) *p-- = '\0';

                        // // cherche l'app dans la table
                        // int found = 0;
                        // for (int i = 0; apps[i].name; i++) {
                        //     printf("  [DBG] comparing '%s' to '%s'\n", arg, apps[i].name);
                        //     if (!strcmp(arg, apps[i].name)) {
                        //         apps[i].fn();
                        //         found = 1;
                        //         break;
                        //     }
                        // }
                        // if (!found) {
                        //     printf("App inconnue: '%s'\n", arg);
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
                    // lance_runner();
                    // console_putchar('\n');

                } else if (strcmp(cmd, "poweroff") == 0) {
                    shutdown(1);
                    console_putchar('\n');

                } else {
                    printf("Commande inconnue: %s\n", cmd);
                }
            }
            // reset
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

