#ifndef __SYSCALL_DEFS_H__
#define __SYSCALL_DEFS_H__

#include <n7OS/processus.h>

#define NB_SYSCALL 10

// Numéros des appels système
#define NR_example 0
#define NR_shutdown 1
#define NR_write 2
#define NR_getchar 3
#define NR_fork 4
#define NR_exit 5
#define NR_kill 6
#define NR_getpid 7
#define NR_sleep 8
#define NR_wait 9


// Déclaration des fonctions d'appels système
int sys_example();

int sys_shutdown(int n);

int sys_write(const char *s, int len);

int sys_fork(const char *name, void (*fun)());

int sys_exit();

int sys_kill(pid_t pid);

int sys_getpid();

int sys_sleep(int seconds);

int sys_wait(pid_t *pid);

int sys_getchar();

// Type pour les pointeurs de fonction d'appels système
typedef int (*fn_ptr)();

extern fn_ptr syscall_table[NB_SYSCALL];

// Fonction pour ajouter un appel système à la table
void add_syscall(int num, fn_ptr function);

#endif
