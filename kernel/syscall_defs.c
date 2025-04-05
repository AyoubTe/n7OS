#include <n7OS/syscall_defs.h>

// Tableau des pointeurs vers les fonctions d'appels système
fn_ptr syscall_table[NB_SYSCALL];

// Fonction pour ajouter un appel système à la table
void add_syscall(int num, fn_ptr function) {
  if (num < NB_SYSCALL) {
    syscall_table[num]= function;
  }
}
