//
// Created by Ayoub SAMI on 03/04/2025.
//

#include <n7OS/processus.h>
#include <n7OS/cpu.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <debug.h>

extern void kernel_start(); // point d'entrée

extern void ctx_sw(uint32_t *old_regs, uint32_t *new_regs);

/* Déclarations des tableaux et variables nécessaires */
// Tableau des processus
struct Process_t process_table[MAX_PROCESS] = { [0 ... (MAX_PROCESS - 1)] = { .state = TERMINE } };

// Tableau des processus prêts
pid_t readyQueue[MAX_PROCESS];
int readyQueueTail = 0;

// Tableau des ressources
pid_t resourceQueue[MAX_PROCESS][MAX_RESOURCE_QUEUE];
int resourceQueueTail[MAX_PROCESS] = {0};

pid_t creer_p(const char *name, void (*function)()) {
    pid_t pid = -1;

    // Trouver un PID libre
    // 1) trouver un slot libre
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
    // 2) init méta
    process_table[pid].pid = pid;
    process_table[pid].name = name;
    process_table[pid].state = PRET_ACTIF;
    process_table[pid].function = function;
    process_table[pid].priority = 0;

    // 3) préparer la pile
    uint32_t *stack = process_table[pid].stack;
    // en sommet, mettez l'adresse de la fonction (ret simulé)
    stack[STACK_SIZE - 1] = (uint32_t)function;

    // 4) contexte initial tous les regs à 0 : regs = { ebx=0, esp=&stack[top], ebp=0, esi=0, edi=0 }
    for (int r = 0; r < 5; r++) {
        process_table[pid].regs[r] = 0;
    }

    // regs[1] <=> ESP doit pointer sur *stack[top] (pointer ESP sur ce sommet)
    process_table[pid].regs[1] = (uint32_t)&stack[STACK_SIZE - 1];


    // 5) en mettre en file des prêts
    addProcess(pid);

    return pid;
}


void activer_p(pid_t pid) {
    if (process_table[pid].state == PRET_SUSPENDU) {
        process_table[pid].state = PRET_ACTIF;
        addProcess(pid); // Ajouter à la file des prêts
    } else if (process_table[pid].state == BLOQUE_SUSPENDU) {
        process_table[pid].state = BLOQUE_ACTIF;
    }
}

void suspendre_p(pid_t pid) {
    if (process_table[pid].state == PRET_ACTIF) {
        process_table[pid].state = PRET_SUSPENDU;
        removeProcess(pid);
    } else if (process_table[pid].state == BLOQUE_ACTIF) {
        process_table[pid].state = BLOQUE_SUSPENDU;
    }
}

void arreter_p() {
    pid_t pid = getpid_p(); // PID du processus en cours
    if (process_table[pid].state == ELU) {
        process_table[pid].state = PRET_ACTIF;
        addProcess(pid);
        schedule(); // Passer la main à un autre processus
    }
}

void bloquer_p(uint32_t rid) {
    pid_t pid = getpid_p();
    if (process_table[pid].state == ELU) {
        process_table[pid].state = BLOQUE_ACTIF;
        addResource(rid, pid); // Ajouter à la file de la ressource
        schedule();
    }
}

void debloquer_p(uint32_t rid) {
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
pid_t getpid_p() {
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

/* Gestion des appels système */
pid_t fork_p(const char *name, void (*fun)()){
    return creer_p(name, fun);
}

int exit_p(){
    pid_t pid = getpid_p(); // PID du processus en cours
    schedule(); // Passer la main à un autre processus
    process_table[pid].state = TERMINE; // Marquer le processus comme terminé
    removeProcess(pid); // Retirer de la file des prêts
    return 0; // Success
}

int kill_p(pid_t pid) {
    if ((int)pid < 0 || pid >= MAX_PROCESS || process_table[pid].state == TERMINE) {
        return -1; // Erreur : processus inexistant
    }
    process_table[pid].state = TERMINE; // Marquer le processus comme terminé
    removeProcess(pid); // Retirer de la file des prêts si nécessaire
    return 0; // Success
}

int sleep_p(int seconds) {
    pid_t pid = getpid_p(); // Récupérer le processus en cours
    bloquer_p(pid); // Bloquer le processus
    // Simuler une attente en fonction des secondes
    for (volatile int i = 0; i < (seconds * 1000000); i++); // Boucle occupée simulant un délai (peut être implémenté par un timer)
    debloquer_p(pid); // Débloquer le processus une fois le temps écoulé
    return 0;
}

int wait_p(pid_t *pid) {
    pid_t current_pid = getpid_p();
    if ((int)current_pid < 0) {
        return -1;
    }

    // Bloquer tant que le processus spécifié n'est pas terminé
    while ((int)(*pid) != -1 && process_table[*pid].state != TERMINE) {
        bloquer_p(0); // Bloquer tant que le processus enfant s'exécute
    }

    return 0;
}


void schedule() {
    pid_t current = getpid_p();
    
    // 1) chercher le premier prêt différent de kernel (0)
    pid_t next = -1;
    for (int i = 1; i < MAX_PROCESS; i++) {
        if (process_table[i].state == PRET_ACTIF) {
            next = i;
            break;
        }
    }

    // 2) Si aucun autre prêt, on reste sur cur (kernel reste ELU)
    if ((int)next < 0) {
        next = 0;  // idle
    }

    // 3) Sinon, on veut basculer vers next
    if (next == current) {
        return;
    } else {
        //    a) Si current != 0 (noyau), on démote current en PRET_ACTIF
        process_table[current].state = PRET_ACTIF;
        //    b) On nomme next élu
        process_table[next].state = ELU;
        //    c) Échange de contexte :
        ctx_sw(process_table[current].regs, process_table[next].regs);
    }
}


// static void idle() {
//     while (1) {
//         hlt();     // met le CPU en low-power en attendant la prochaine IT
//     }
// }

void dummyProcess() {
    printf("Execution du processus factice.\n");
    exit_p();
}

void test_processus() {
  	pid_t pid = creer_p("test", dummyProcess);
    assert((int)pid >= 0);
    printf("Processus cree avec PID %d.\n", pid);
    // Vérifier l'état initial du processus (attendu PRET_ACTIF)
    assert(process_table[pid].state == PRET_ACTIF);

    // Suspendre le processus et vérifier son état
    suspendre_p(pid);
    assert(process_table[pid].state == PRET_SUSPENDU);
    printf("Processus suspendu.\n");

    // Activer le processus et vérifier son état
    activer_p(pid);
    assert(process_table[pid].state == PRET_ACTIF);
    printf("Processus active.\n");

    printf("Test de gestion des processus reussi.\n");
}


void init_processus() {

    // Le kernel (PID 0)
    pid_t pid = 0;
    // Remplir le slot 0
    process_table[pid].pid       = 0;
    process_table[pid].name      = "kernel";
    process_table[pid].state     = ELU;
    process_table[pid].priority  = 0;
    process_table[pid].function  = kernel_start; 

    // Récupérer ESP actuel pour démarrer le contexte
    uint32_t esp;
    __asm__ volatile("mov %%esp, %0" : "=r" (esp));
    process_table[pid].regs[1] = esp;  // regs[1] = esp

    // // PID 1 = idle
    // pid = creer_p("idle", idle);
    // // on peut lui donner la priorité la plus basse
    // process_table[pid].priority = 1;
}
