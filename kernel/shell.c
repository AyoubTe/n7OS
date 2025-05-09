#include <n7OS/console.h>
#include <n7OS/processus.h>
#include <n7OS/keyboard.h>
#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <string.h>
#include <stdio.h>

#define MAX_LINE 80

extern void runner_loop(void);


// La fonction shell_run :
int lance_runner() {
    // Crée un processus « runner »
    pid_t pid = creer_p("runner", runner_loop);
    if ((int)pid >= 0) process_table[pid].priority = 10;  // priorité max
    if ((int)pid < 0) {
        printf("Impossible de démarrer le jeu\n");
        return -1;
    }
    return 0;
}


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
                    printf("clear: efface l'ecran\n");
                    printf("run <nom app>: lance l'app\n");
                    printf("poweroff: arrete le systeme\n");
                } else if (strcmp(cmd, "clear") == 0) {
                    console_putbytes("\f", 1);
                } else if (strcmp(cmd, "run") == 0) {
                    if(strcmp(arg, "runner")){
                        lance_runner();
                        console_putchar('\n');
                    } else {
                        printf("App inconnue: %s\n", arg);
                    }
                    
                } else if (strcmp(cmd, "poweroff") == 0) {
                    sys_shutdown(1);
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

