//
// Created by Ayoub SAMI on 04/04/2025.
//

// syscall.h
#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <n7OS/processus.h>

// Wrappers pour les appels système

// Appel système de test
static inline int example() {
    int result;
    __asm__ volatile(
      "movl $0, %%eax\n"
      "int $0x80\n"
      "movl %%eax, %0"
      : "=r" (result)
      :
      : "eax"
    );
    return result;
}

// Création d'un processus
static inline pid_t fork(const char *name, void (*fun)()) {
    pid_t result;
    __asm__ volatile(
      "movl $1, %%eax\n"
      "movl %1, %%ebx\n"
      "movl %2, %%ecx\n"
      "int $0x80\n"
      "movl %%eax, %0"
      : "=r" (result)
      : "r" (name), "r" (fun)
      : "eax", "ebx", "ecx"
    );
    return result;
}

// Terminaison du processus courant
static inline int exit_process() {
    int result;
    __asm__ volatile(
      "movl $2, %%eax\n"
      "int $0x80\n"
      "movl %%eax, %0"
      : "=r" (result)
      :
      : "eax"
    );
    return result;
}

// Tuer un processus
static inline int kill(pid_t pid) {
    int result;
    __asm__ volatile(
      "movl $3, %%eax\n"
      "movl %1, %%ebx\n"
      "int $0x80\n"
      "movl %%eax, %0"
      : "=r" (result)
      : "r" (pid)
      : "eax", "ebx"
    );
    return result;
}

// Obtenir l'identifiant du processus courant
static inline pid_t getpid() {
    pid_t result;
    __asm__ volatile(
      "movl $4, %%eax\n"
      "int $0x80\n"
      "movl %%eax, %0"
      : "=r" (result)
      :
      : "eax"
    );
    return result;
}

// Mettre en sommeil le processus courant
static inline int sleep(int seconds) {
    int result;
    __asm__ volatile(
      "movl $5, %%eax\n"
      "movl %1, %%ebx\n"
      "int $0x80\n"
      "movl %%eax, %0"
      : "=r" (result)
      : "r" (seconds)
      : "eax", "ebx"
    );
    return result;
}

// Attendre la fin d'un processus
static inline int wait(pid_t *pid) {
    int result;
    __asm__ volatile(
      "movl $6, %%eax\n"
      "movl %1, %%ebx\n"
      "int $0x80\n"
      "movl %%eax, %0"
      : "=r" (result)
      : "r" (pid)
      : "eax", "ebx"
    );
    return result;
}

#endif