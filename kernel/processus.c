//
// Created by Ayoub SAMI on 03/04/2025.
//

#include <n7OS/processus.h>
#include <n7OS/cpu.h>
#include <inttypes.h>
#include <stdio.h>
#include <debug.h>


/* Déclarations des tableaux et variables nécessaires */
// Tableau des processus
struct Process_t process_table[MAX_PROCESS] = { [0 ... (MAX_PROCESS - 1)] = { .state = TERMINE } };

// Tableau des processus prêts
pid_t readyQueue[MAX_PROCESS];
int readyQueueTail = 0;

// Tableau des ressources
pid_t resourceQueue[MAX_PROCESS][MAX_RESOURCE_QUEUE];
int resourceQueueTail[MAX_PROCESS] = {0};

pid_t creer(const char *name, void (*function)()) {
    pid_t pid = -1;

    // Trouver un PID libre
    for (uint32_t i = 0; i < MAX_PROCESS; i++) {
        if (process_table[i].state == TERMINE) {
            pid = i;
            break;
        }
    }

    if ((int)pid == -1) {
        printf("Erreur : impossible de creer un nouveau processus\n");
        return -1;
    }

    // Initialiser le processus
    process_table[pid].pid = pid;
    process_table[pid].name = name;
    process_table[pid].state = PRET_ACTIF;
    process_table[pid].stack_top = STACK_SIZE - 1;
    process_table[pid].function = function;

    // Initialiser les registres (contexte)
    for (uint32_t i = 0; i < 5; i++) {
        process_table[pid].regs[i] = 0;
    }
    process_table[pid].regs[1] = (uint32_t)&process_table[pid].stack[STACK_SIZE - 1]; // esp

    // Ajouter à la file des processus prêts
    addProcess(pid);

    return pid;
}


void activer(pid_t pid) {
    if (process_table[pid].state == PRET_SUSPENDU) {
        process_table[pid].state = PRET_ACTIF;
        addProcess(pid); // Ajouter à la file des prêts
    } else if (process_table[pid].state == BLOQUE_SUSPENDU) {
        process_table[pid].state = BLOQUE_ACTIF;
    }
}

void suspendre(pid_t pid) {
    if (process_table[pid].state == PRET_ACTIF) {
        process_table[pid].state = PRET_SUSPENDU;
        removeProcess(pid);
    } else if (process_table[pid].state == BLOQUE_ACTIF) {
        process_table[pid].state = BLOQUE_SUSPENDU;
    }
}

void arreter() {
    pid_t pid = getpid(); // PID du processus en cours
    if (process_table[pid].state == ELU) {
        process_table[pid].state = PRET_ACTIF;
        addProcess(pid);
        schedule(); // Passer la main à un autre processus
    }
}

void bloquer(uint32_t rid) {
    pid_t pid = getpid();
    if (process_table[pid].state == ELU) {
        process_table[pid].state = BLOQUE_ACTIF;
        addResource(rid, pid); // Ajouter à la file de la ressource
        schedule();
    }
}

void debloquer(uint32_t rid) {
    // Parcourir les processus liés à la ressource et les débloquer
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (isProcessWaitingForResource(rid, process_table[i].pid)) {
            if (process_table[i].state == BLOQUE_ACTIF) {
                process_table[i].state = PRET_ACTIF;
                addProcess(process_table[i].pid);
            } else if (process_table[i].state == BLOQUE_SUSPENDU) {
                process_table[i].state = PRET_SUSPENDU;
            }
        }
    }
}

// Récuperer le PID du processus en cours, ici on fait de round robin
pid_t getpid() {
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (process_table[i].state == ELU) {
            return process_table[i].pid;
        }
    }
    return -1; // Aucun processus élu
}

void addProcess(pid_t pid) {
    // Ajout du processus à la file des prêts
    enqueueReadyProcess(pid);
}

void removeProcess(pid_t pid) {
    // Retrait du processus de la file des prêts
    dequeueReadyProcess(pid);
}

void addResource(uint32_t rid, pid_t pid) {
    // Ajout du processus à la file d’attente pour une ressource
    enqueueResource(rid, pid);
}

int isProcessWaitingForResource(uint32_t rid, pid_t pid) {
    // Vérifier si un processus attend une ressource
    return checkResourceQueue(rid, pid);
}

void schedule() {
    pid_t current_pid = getpid();
    pid_t next_pid = -1;

    // Sauvegarder le contexte du processus actuel si actif
    if ((int)current_pid != -1 && process_table[current_pid].state == ELU) {
        process_table[current_pid].state = PRET_ACTIF;
    }

    // Trouver le prochain processus prêt
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (process_table[i].state == PRET_ACTIF) {
            next_pid = i;
            break;
        }
    }

    if ((int)next_pid == -1) {
        printf("Aucun processus pret\n");
        return;
    }

    // Changer de contexte
    process_table[next_pid].state = ELU;
    ctx_sw(process_table[current_pid].regs, process_table[next_pid].regs);
}


void enqueueReadyProcess(pid_t pid) {
    // Implémentation de l'ajout d'un processus à la file des processus prêts
    readyQueue[readyQueueTail] = pid;
    readyQueueTail = (readyQueueTail + 1) % MAX_PROCESS;
}

void dequeueReadyProcess(pid_t pid) {
    // Implémentation du retrait d'un processus de la file des processus prêts
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (readyQueue[i] == pid) {
            readyQueue[i] = -1; // Marqueur pour indiquer une place libre
            break;
        }
    }
}

void enqueueResource(uint32_t rid, pid_t pid) {
    // Ajout d'un processus à la file d'attente de la ressource
    resourceQueue[rid][resourceQueueTail[rid]] = pid;
    resourceQueueTail[rid] = (resourceQueueTail[rid] + 1) % MAX_RESOURCE_QUEUE;
}

int checkResourceQueue(uint32_t rid, pid_t pid) {
    // Vérifier si un processus est dans la file d'attente de la ressource
    for (int i = 0; i < MAX_RESOURCE_QUEUE; i++) {
        if (resourceQueue[rid][i] == pid) {
            return 1;
        }
    }
    return 0;
}

void dummyProcess() {
    printf("Execution du processus factice.\n");
}

void test_processus() {
  	pid_t pid = creer("test", dummyProcess);

    assert((int)pid >= 0);
    printf("Processus cree avec PID %d.\n", pid);

    // Vérifier l'état initial du processus (attendu PRET_ACTIF)
    assert(process_table[pid].state == PRET_ACTIF);

    // Suspendre le processus et vérifier son état
    suspendre(pid);
    assert(process_table[pid].state == PRET_SUSPENDU);
    printf("Processus suspendu.\n");

    // Activer le processus et vérifier son état
    activer(pid);
    assert(process_table[pid].state == PRET_ACTIF);
    printf("Processus active.\n");

    // Si nécessaire, simuler la planification pour observer le changement de contexte
    schedule();
    // Ici nous pouvons ajouter des vérifications supplémentaires après changement de contexte

    printf("Test de gestion des processus reussi.\n");
}


/* Gestion des appels système */
pid_t fork(const char *name, void (*fun)()){
    return creer(name, fun);
}

int exit_process(){
    pid_t pid = getpid(); // PID du processus en cours
    process_table[pid].state = TERMINE; // Marquer le processus comme terminé
    removeProcess(pid); // Retirer de la file des prêts
    schedule(); // Passer la main à un autre processus
    return 0; // Success
}

int kill(pid_t pid) {
    if ((int)pid < 0 || pid >= MAX_PROCESS || process_table[pid].state == TERMINE) {
        return -1; // Erreur : processus inexistant
    }
    process_table[pid].state = TERMINE; // Marquer le processus comme terminé
    removeProcess(pid); // Retirer de la file des prêts si nécessaire
    return 0; // Success
}

int sleep(int seconds) {
    pid_t pid = getpid(); // Récupérer le processus en cours
    bloquer(pid); // Bloquer le processus
    // Simuler une attente en fonction des secondes
    for (volatile int i = 0; i < (seconds * 1000000); i++); // Boucle occupée simulant un délai (peut être implémenté par un timer)
    debloquer(pid); // Débloquer le processus une fois le temps écoulé
    return 0;
}

int wait(pid_t *pid) {
    pid_t current_pid = getpid();
    if ((int)current_pid < 0) {
        return -1;
    }

    // Bloquer tant que le processus spécifié n'est pas terminé
    while (*pid != -1 && process_table[*pid].state != TERMINE) {
        bloquer(0); // Bloquer tant que le processus enfant s'exécute
    }

    return 0;
}

void test_syscall() {
    printf("Test des appels système\n");

    pid_t pid = fork("test_fork", dummyProcess);
    if ((int)pid > 0) {
        printf("Parent : processus forké avec PID %d\n", pid);
        wait(&pid);
        printf("Parent : processus enfant %d terminé\n", pid);
    } else if ((int)pid == 0) {
        printf("Enfant : je suis un processus enfant\n");
    }

    sleep(1);
    printf("Fin du test des appels système\n");
}
