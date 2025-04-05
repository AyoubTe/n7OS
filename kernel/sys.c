#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <n7OS/console.h>
#include <n7OS/irq.h>
#include <unistd.h>

extern void handler_syscall();

void init_syscall() {
  // ajout de la fonction de traitement de l'appel systeme
  add_syscall(NR_example, sys_example);
  add_syscall(SYS_FORK, (fn_ptr)sys_fork);
  add_syscall(SYS_EXIT, (fn_ptr)sys_exit);
  add_syscall(SYS_KILL, (fn_ptr)sys_kill);
  add_syscall(SYS_GETPID, (fn_ptr)sys_getpid);
  add_syscall(SYS_SLEEP, (fn_ptr)sys_sleep);
  add_syscall(SYS_WAIT, (fn_ptr)sys_wait);

  // initialisation de l'IT soft qui gère les appels systeme
  init_irq_entry(0x80, (uint32_t) handler_syscall);
}

// code de la fonction de traitement de l'appel systeme example
int sys_example() {
  // on ne fait que retourner 1
  return 1;
}

// Implémentations des appels système de gestion des processus
pid_t sys_fork(const char *name, void (*fun)()) {
  return fork(name, fun);
}

int sys_exit() {
  return exit_process();
}

int sys_kill(pid_t pid) {
  return kill(pid);
}

pid_t sys_getpid() {
  return getpid();
}

int sys_sleep(int seconds) {
  return sleep(seconds);
}

int sys_wait(pid_t *pid) {
  return wait(pid);
}

