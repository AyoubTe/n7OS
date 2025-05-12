#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <n7OS/console.h>
#include <n7OS/irq.h>
#include <n7OS/cpu.h>
#include <n7OS/processus.h>
#include <n7OS/keyboard.h>
#include <unistd.h>

/**
 * @file sys.c
 *
 * @brief Contains kernel system call functions.
 *
 * Note: The functions in this file are currently implemented as placeholders.
 * They do not perform any real operations; they simply return a constant value.
*/

extern void handler_syscall();


/**
 * @brief Dummy system call.
 *
 * This function serves as a placeholder for the actual system call handler.
 * It does not perform any computation or system operations.
 *
 * @return int Always returns 1.
*/
// code de la fonction de traitement de l'appel systeme example
int sys_example() {
  // on ne fait que retourner 1
  return 1;
}

/**
 * sys_fork - Create a new process.
 *
 * This function forks a new process that will begin execution at the specified function.
 *
 * @name: A string representing the process name.
 * @fun: A pointer to the function that the child process will execute.
 *
 * Returns the process identifier (PID) of the newly created process on success,
 * or a negative value on failure.
 */
// Implémentations des appels système de gestion des processus
int sys_fork(const char *name, void (*fun)()) {
  return fork_p(name, fun);
}


/**
 * sys_exit - Terminate the current process.
 *
 * This function terminates the execution of the calling process.
 *
 * Returns a status code that may indicate an error depending on implementation details.
 */
int sys_exit() {
  return exit_p();
}

/**
 * sys_kill - Terminate a process by its PID.
 *
 * This function sends a termination request to the process identified by the given PID.
 *
 * @pid: The process identifier of the target process to be terminated.
 *
 * Returns 0 on success or a negative value on failure.
 */
int sys_kill(pid_t pid) {
  return kill_p(pid);
}


/**
 * sys_getpid - Retrieve the PID of the current process.
 *
 * This function returns the process identifier of the calling process.
 *
 * Returns the current process's PID.
 */
int sys_getpid() {
  return getpid_p();
}


/**
 * sys_sleep - Suspend process execution for a specified duration.
 *
 * This function puts the process to sleep for the given number of seconds.
 *
 * @seconds: The number of seconds for which the process should sleep.
 *
 * Returns a status code indicating success or failure.
 */
int sys_sleep(int seconds) {
  return sleep_p(seconds);
}


/**
 * sys_wait - Wait for a process to change state.
 *
 * This function causes the calling process to wait until one of its child processes
 * terminates or changes state, storing the child's PID in the provided location.
 *
 * @pid: A pointer to store the PID of the child process whose state has changed.
 *
 * Returns a status code indicating success or the nature of any error encountered.
 */
int sys_wait(pid_t *pid) {
  return wait_p(pid);
}


/**
 * sys_shutdown - Shutdown the system.
 *
 * This function initiates a system shutdown if the provided parameter indicates completion.
 * A special hardware command is sent if the parameter corresponds to QEMU shutdown criteria.
 *
 * @n: A value used to determine whether the shutdown procedure should be triggered.
 *
 * Returns -1 after initiating the shutdown, otherwise returns the provided parameter.
 */
int sys_shutdown(int n) {
  if (n == 1) {
    /* Envoie d'un mot à QEMU pour forcer l'arrêt (QEMU > 2.0) */
    outw(0x2000, 0x604); // Poweroff qemu > 2.0
    return-1;
  } else
    return n;
}


/**
 * sys_write - Write a string to the console.
 *
 * This function sends a string to the console output via the underlying console mechanism.
 *
 * @s: A pointer to the string to be displayed.
 * @len: The number of characters from the string to write.
 *
 * Returns the number of characters written.
 */
/* Affiche la chaîne s sur la console à l'aide de console_putbytes */
int sys_write(const char *s, int len) {
  console_putbytes(s, len);
  return len;
}


/**
 * sys_getchar - Get a character from the keyboard.
 *
 * This function retrieves a character input from the keyboard.
 *
 * Returns the character code received from the keyboard input.
 */
/* Implémentation de sys_getchar pour la lecture clavier */
int sys_getchar(void) {
  return kgetch();
}


/**
 * init_syscall - Initialize system calls.
 *
 * This function registers various system call handlers for process management,
 * console I/O, and other basic system functionalities by adding them to the system's
 * syscall table. It also sets up the corresponding interrupt descriptor for handling
 * system calls.
 */
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

