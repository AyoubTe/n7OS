#ifndef __SYSCALL_DEFS_H__
#define __SYSCALL_DEFS_H__

#include <n7OS/processus.h>

#define NB_SYSCALL 7

// Numéros des appels système
#define SYS_FORK       1
#define SYS_EXIT       2
#define SYS_KILL       3
#define SYS_GETPID     4
#define SYS_SLEEP      5
#define SYS_WAIT       6

// Déclaration des fonctions d'appels système
int sys_example();

pid_t sys_fork(const char *name, void (*fun)());
int sys_exit();
int sys_kill(pid_t pid);
pid_t sys_getpid();
int sys_sleep(int seconds);
int sys_wait(pid_t *pid);

// Type pour les pointeurs de fonction d'appels système
typedef int (*fn_ptr)();
extern fn_ptr syscall_table[NB_SYSCALL];

// Fonction pour ajouter un appel système à la table
void add_syscall(int num, fn_ptr function);

#endif
