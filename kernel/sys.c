#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <n7OS/console.h>
#include <n7OS/irq.h>
#include <n7OS/cpu.h>
#include <n7OS/processus.h>
#include <n7OS/keyboard.h>
#include <unistd.h>

extern void handler_syscall();


// code de la fonction de traitement de l'appel systeme example
int sys_example() {
  // on ne fait que retourner 1
  return 1;
}

// Implémentations des appels système de gestion des processus
int sys_fork(const char *name, void (*fun)()) {
  return fork_p(name, fun);
}

int sys_exit() {
  return exit_p();
}

int sys_kill(pid_t pid) {
  return kill_p(pid);
}

int sys_getpid() {
  return getpid_p();
}

int sys_sleep(int seconds) {
  return sleep_p(seconds);
}

int sys_wait(pid_t *pid) {
  return wait_p(pid);
}


int sys_shutdown(int n) {
  if (n == 1) {
    /* Envoie d'un mot à QEMU pour forcer l'arrêt (QEMU > 2.0) */
    outw(0x2000, 0x604); // Poweroff qemu > 2.0
    return-1;
  } else
    return n;
}


/* Affiche la chaîne s sur la console à l'aide de console_putbytes */
int sys_write(const char *s, int len) {
  console_putbytes(s, len);
  return len;
}

/* Implémentation de sys_getchar pour la lecture clavier */
int sys_getchar(void) {
  return kgetch();
}


void init_syscall() {
  // ajout de la fonction de traitement de l'appel systeme i.e Ajout des appels système à la table
  add_syscall(NR_example, sys_example);
  add_syscall(NR_shutdown, sys_shutdown);
  add_syscall(NR_write, sys_write);
  add_syscall(NR_getchar, sys_getchar);
  // Fonctions sys pour gestion des processus
  add_syscall(NR_fork, sys_fork);
  add_syscall(NR_exit, sys_exit);
  add_syscall(NR_kill, sys_kill);
  add_syscall(NR_getpid, sys_getpid);
  add_syscall(NR_sleep, sys_sleep);
  add_syscall(NR_wait, sys_wait);  

  // initialisation de l'IDT pour les appels systeme
  init_irq_entry(0x80, (uint32_t) handler_syscall);
}

