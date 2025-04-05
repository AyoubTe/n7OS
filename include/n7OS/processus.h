//
// Created by Ayoub SAMI on 03/04/2025.
//

#ifndef PROCESSUS_H
#define PROCESSUS_H

#include <inttypes.h>

#define MAX_PROCESS 156 // Nombre maximal de processus
#define STACK_SIZE 1024  // Taille de la pile d'exécution d'une fonction (exemple : 1 page)
#define MAX_RESOURCE_QUEUE 500 // Nombre maximal de processus en attente d'un ressource

// Identifiants de processus et de ressources
typedef uint32_t pid_t;
typedef uint32_t rid_t;

// Etats des processus
typedef enum {
    ELU, PRET_ACTIF, PRET_SUSPENDU, BLOQUE_ACTIF, BLOQUE_SUSPENDU, TERMINE
} PROCESS_STATE;

// Structure des processus
struct Process_t {
    const char *name;               // Nom du programme (Processus)
    pid_t pid;                // Identifiant unique
    PROCESS_STATE state;      // État du processus
    uint32_t priority;        // Priorité
    uint32_t stack[STACK_SIZE]; // Pile d'exécution
    uint32_t stack_top;       // Sommet de la pile
    uint32_t regs[5];         // Contexte : ebx, esp, ebp, esi, edi
    void (*function)();       // Pointeur vers la fonction à exécuter
};

/* Définir les appels systèmes: */

pid_t fork(const char *name, void (*fun)());

int exit_process();

int kill(pid_t pid);

pid_t getpid();

int sleep(int seconds);

int wait(pid_t *pid);


/* Les fonctions de gestion des processus */
extern struct Process_t process_table[MAX_PROCESS]; // Tableau de processus

pid_t creer(const char *name, void (*function)());

void activer(pid_t pid);

void suspendre(pid_t pid);

void arreter();

void bloquer(uint32_t rid);

void schedule();

void addProcess(pid_t pid);

void removeProcess(pid_t pid);

void addResource(rid_t rid, pid_t pid);

void ctx_sw(uint32_t *old_regs, uint32_t *new_regs);

void enqueueReadyProcess(pid_t pid);

void init_processus();

void enqueueResource(uint32_t rid, pid_t pid);

void dequeueReadyProcess(pid_t pid);

int checkResourceQueue(uint32_t rid, pid_t pid);

int isProcessWaitingForResource(uint32_t rid, pid_t pid);

void test_processus();

#endif