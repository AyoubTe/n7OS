
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

//extern void shell_loop(void);

/* Déclarations des tableaux et variables nécessaires */
// Tableau des processus
struct Process_t process_table[MAX_PROCESS] = { [0 ... (MAX_PROCESS - 1)] = { .state = TERMINE } };

// Tableau des processus prêts
pid_t readyQueue[MAX_PROCESS];
int readyQueueTail = 0;

// Tableau des ressources
pid_t resourceQueue[MAX_PROCESS][MAX_RESOURCE_QUEUE];
int resourceQueueTail[MAX_PROCESS] = {0};


/**
 * creer_p - Create a new process.
 * @name: The name of the process.
 * @function: The entry point function for the process.
 *
 * Scans the process table for a free PID. If a free slot is found, initializes the
 * process meta-data with its ID, name, state, and function pointer. Sets up its stack
 * with a simulated return pointer, initializes its registers, and enqueues it in the
 * ready queue.
 *
 * Returns: The PID of the newly created process on success, or -1 on failure.
 */
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
    //process_table[pid].priority = 0;

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


/**
 * activer_p - Activate or resume a suspended process.
 * @pid: The process ID to be activated.
 *
 * If the process state indicates it is suspended (either active or blocked variant),
 * this function changes its state to the corresponding active state. For active suspended
 * processes, it re-adds the process to the ready queue.
 */
void activer_p(pid_t pid) {
    if (process_table[pid].state == PRET_SUSPENDU) {
        process_table[pid].state = PRET_ACTIF;
        addProcess(pid); // Ajouter à la file des prêts
    } else if (process_table[pid].state == BLOQUE_SUSPENDU) {
        process_table[pid].state = BLOQUE_ACTIF;
    }
}


/**
 * suspendre_p - Suspend an active process.
 * @pid: The process ID of the process to be suspended.
 *
 * Depending on its current state (active or blocked active), the process state is updated
 * to its corresponding suspended variant. If the process was active, it is removed from the
 * ready queue.
 */
void suspendre_p(pid_t pid) {
    if (process_table[pid].state == PRET_ACTIF) {
        process_table[pid].state = PRET_SUSPENDU;
        removeProcess(pid);
    } else if (process_table[pid].state == BLOQUE_ACTIF) {
        process_table[pid].state = BLOQUE_SUSPENDU;
    }
}


/**
 * arreter_p - Terminate the currently running process.
 *
 * Marks the current process as terminated, resets its priority, and removes it from the
 * ready queue. It then prints a prompt and invokes the scheduler to switch execution to
 * another process.
 *
 * Returns: 0 upon successful termination.
 */
void arreter_p() {
    pid_t pid = getpid_p(); // PID du processus en cours
    if (process_table[pid].state == ELU) {
        process_table[pid].state = PRET_ACTIF;
        addProcess(pid);
        schedule(); // Passer la main à un autre processus
    }
}


/**
 * bloquer_p - Block the currently executing process with respect to a resource.
 * @rid: The identifier of the resource for which the process is being blocked.
 *
 * Changes the state of the current process to blocked active, enqueues it on the waiting
 * queue for the specified resource, and then calls the scheduler to perform a context
 * switch.
 */
void bloquer_p(uint32_t rid) {
    pid_t pid = getpid_p();
    if (process_table[pid].state == ELU) {
        process_table[pid].state = BLOQUE_ACTIF;
        addResource(rid, pid); // Ajouter à la file de la ressource
        schedule();
    }
}


/**
 * debloquer_p - Unblock all processes waiting for a specific resource.
 * @rid: The identifier of the resource.
 *
 * Iterates through the process table to check which processes are waiting for the specified
 * resource. For each matching process that is blocked active, its state is changed to active
 * and enqueued in the ready queue. If a process is in a suspended blocked state, it is set to
 * suspended.
 */
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


/**
 * getpid_p - Retrieve the process identifier (PID) of the currently executing process.
 *
 * Searches the process table for the process whose state is marked as elected (ELU).
 *
 * Returns: The PID of the current process, or -1 if no such process is found.
 */
// Récuperer le PID du processus en cours, ici on fait de round robin
pid_t getpid_p() {
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (process_table[i].state == ELU) {
            return process_table[i].pid;
        }
    }
    return -1; // Aucun processus élu
}


/**
 * addProcess - Enqueue a process into the ready process queue.
 * @pid: The process ID to be enqueued.
 *
 * This function adds the given process ID to the ready queue, making it eligible for scheduling.
 */
void addProcess(pid_t pid) {
    // Ajout du processus à la file des prêts
    enqueueReadyProcess(pid);
}


/**
 * removeProcess - Remove a process from the ready process queue.
 * @pid: The process ID to be removed.
 *
 * Searches the ready queue for the given process and marks its slot as free to indicate removal.
 */
void removeProcess(pid_t pid) {
    // Retrait du processus de la file des prêts
    dequeueReadyProcess(pid);
}


/**
 * addResource - Enqueue a process into a specific resource's waiting queue.
 * @rid: The resource identifier.
 * @pid: The process ID to be added to the resource's waiting queue.
 *
 * Adds the specified process to the queue tracking processes that are waiting for the given
 * resource.
 */
void addResource(uint32_t rid, pid_t pid) {
    // Ajout du processus à la file d’attente pour une ressource
    enqueueResource(rid, pid);
}


/**
 * isProcessWaitingForResource - Check if a process is waiting for a particular resource.
 * @rid: The resource identifier.
 * @pid: The process ID to check within the resource's waiting queue.
 *
 * Iterates over the resource-specific queue to determine if the process is enqueued.
 *
 * Returns: 1 if the process is found waiting for the resource; otherwise, 0.
 */
int isProcessWaitingForResource(uint32_t rid, pid_t pid) {
    // Vérifier si un processus attend une ressource
    return checkResourceQueue(rid, pid);
}


/**
 * enqueueReadyProcess - Add a process to the ready queue.
 * @pid: The process ID to be added.
 *
 * Implements the addition of the process to the ready queue using a circular buffer mechanism.
 */
void enqueueReadyProcess(pid_t pid) {
    // Implémentation de l'ajout d'un processus à la file des processus prêts
    readyQueue[readyQueueTail] = pid;
    readyQueueTail = (readyQueueTail + 1) % MAX_PROCESS;
}


/**
 * dequeueReadyProcess - Remove a process from the ready queue.
 * @pid: The process ID to be removed.
 *
 * Searches for the process in the ready queue and marks its position with a free marker (-1)
 * to indicate its removal.
 */
void dequeueReadyProcess(pid_t pid) {
    // Implémentation du retrait d'un processus de la file des processus prêts
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (readyQueue[i] == pid) {
            readyQueue[i] = -1; // Marqueur pour indiquer une place libre
            break;
        }
    }
}


/**
 * enqueueResource - Add a process to the waiting queue for the specified resource.
 * @rid: The resource identifier.
 * @pid: The process ID to be enqueued.
 *
 * Enqueues the process into the waiting queue associated with a resource, updating the queue's
 * tail pointer in a circular fashion.
 */
void enqueueResource(uint32_t rid, pid_t pid) {
    // Ajout d'un processus à la file d'attente de la ressource
    resourceQueue[rid][resourceQueueTail[rid]] = pid;
    resourceQueueTail[rid] = (resourceQueueTail[rid] + 1) % MAX_RESOURCE_QUEUE;
}


/**
 * checkResourceQueue - Verify if a process exists in a resource's waiting queue.
 * @rid: The resource identifier.
 * @pid: The process ID to search for.
 *
 * Iterates over the resource queue for the given resource identifier to confirm if the process
 * is waiting.
 *
 * Returns: 1 if the process is found; otherwise, returns 0.
 */
int checkResourceQueue(uint32_t rid, pid_t pid) {
    // Vérifier si un processus est dans la file d'attente de la ressource
    for (int i = 0; i < MAX_RESOURCE_QUEUE; i++) {
        if (resourceQueue[rid][i] == pid) {
            return 1;
        }
    }
    return 0;
}


/**
 * fork_p - Create a new process in a manner similar to forking.
 * @name: The name of the new process.
 * @fun: The entry point function for the new process.
 *
 * This is a wrapper around creer_p that simulates the behavior of a fork by creating a new
 * process with the specified parameters.
 *
 * Returns: The PID of the newly created process on success.
 */
/* Gestion des appels système */
pid_t fork_p(const char *name, void (*fun)()){
    return creer_p(name, fun);
}


/**
 * exit_p - Terminate the current process.
 *
 * Sets the current process's state to terminated, resets its priority, and removes it from the
 * ready queue. After printing a prompt, the scheduler is invoked to switch execution.
 *
 * Returns: 0 upon successful termination.
 */
int exit_p(){
    pid_t pid = getpid_p(); // PID du processus en cours
    process_table[pid].state = TERMINE; // Marquer le processus comme terminé
    process_table[pid].priority = 0;
    removeProcess(pid); // Retirer de la file des prêts
    printf("$ ");
    schedule(); // Passer la main à un autre processus (typiquement ici c'est le shell car on a pas de multi-threadinds le shell est le respnsable de lancer les autres processus ie apps)
    return 0; // Success
}


/**
 * kill_p - Terminate a specified process by its ID.
 * @pid: The process ID of the process to be killed.
 *
 * Validates if the process exists and is active. If so, marks the process as terminated and
 * removes it from the ready queue if necessary.
 *
 * Returns: 0 on success, or -1 if the process does not exist or is already terminated.
 */
int kill_p(pid_t pid) {
    if ((int)pid < 0 || pid >= MAX_PROCESS || process_table[pid].state == TERMINE) {
        return -1; // Erreur : processus inexistant
    }
    process_table[pid].state = TERMINE; // Marquer le processus comme terminé
    removeProcess(pid); // Retirer de la file des prêts si nécessaire
    return 0; // Success
}



/**
 * sleep_p - Suspend the process for a specified duration.
 * @seconds: The duration (in seconds) to sleep.
 *
 * Blocks the current process by invoking the blocking mechanism and simulates a delay using a
 * busy loop. Once the delay has elapsed, the process is unblocked.
 *
 * Returns: 0 upon successful sleep.
 */
int sleep_p(int seconds) {
    pid_t pid = getpid_p(); // Récupérer le processus en cours
    bloquer_p(pid); // Bloquer le processus
    // Simuler une attente en fonction des secondes
    for (volatile int i = 0; i < (seconds * 1000000); i++); // Boucle occupée simulant un délai (peut être implémenté par un timer)
    debloquer_p(pid); // Débloquer le processus une fois le temps écoulé
    return 0;
}


/**
 * wait_p - Wait for the termination of a particular process.
 * @pid: Pointer to the process ID to wait for.
 *
 * Causes the calling process to block until the specified process (typically a child) has
 * terminated. The process remains blocked by repeatedly invoking the blocking mechanism.
 *
 * Returns: 0 on success, or -1 if the calling process PID is invalid.
 */
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



/**
 * schedule_round_robin - Perform a round-robin scheduling among processes.
 *
 * Scans for the next available process in the ready queue (skipping the kernel process) and
 * performs a context switch if a different process is found. If no suitable process is found,
 * continues execution with the current or the idle process.
 */
// schedule_round_robin()
void schedule_round_robin() {
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


/**
 * schedule - Perform scheduling based on process priority.
 *
 * Iterates through the process table to locate the active process with the highest priority.
 * If a different process from the current one is found, a context switch is performed to transfer
 * control to the selected process.
 */
//schedule_priority
void schedule() {
    pid_t cur = getpid_p();

    // 1) Chercher le PRET_ACTIF de priorité max
    pid_t next = -1;

    int best_prio = -1;
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (process_table[i].state == PRET_ACTIF
            && (int)process_table[i].priority > best_prio) {
            best_prio = process_table[i].priority;
            next = i;
        }
    }

    
    // 2) Si aucun autre prêt, on reste sur cur (kernel reste ELU)
    if ((int)next < 0) {
        next = 0;  // idle
    }

    // 3) Sinon, on veut basculer vers next
    if (next == cur) {
        return;
    } else {
        //    a) Si current != 0 (noyau), on démote current en PRET_ACTIF
        process_table[cur].state = PRET_ACTIF;
        //    b) On nomme next élu
        process_table[next].state = ELU;
        //    c) Échange de contexte :
        ctx_sw(process_table[cur].regs, process_table[next].regs);
    }
}



/**
 * dummyProcess - A sample process function used for testing.
 *
 * Prints a message indicating it is running and then terminates by calling exit_p.
 */
// Un processus factice
void dummyProcess() {
    printf("Execution du processus factice.\n");
    exit_p();
}


/**
 * test_processus - Test the process management routines.
 *
 * Demonstrates the creation, suspension, and activation of a process. Uses assertions to check
 * that state transitions occur as expected during the process's lifecycle.
 */
// Test de gestion des processus
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



/**
 * init_processus - Initialize the process management system.
 *
 * Sets up the kernel process with PID 0, initializing its execution context (including
 * saving the current stack pointer). Optionally, additional processes (such as a shell) can be
 * created during initialization.
 */
// Initialisation des processus
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

    // Créer le shell au démarrage
    // Shell PID = 1
    // pid_t shell_pid = creer_p("shell", shell_loop);
    // process_table[shell_pid].priority = 10;  // priorité plus grand que le kernel
}
