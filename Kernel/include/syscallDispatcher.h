#ifndef SYSCALL_DISPATCHER_H
#define SYSCALL_DISPATCHER_H

#include <stdint.h>

/* Typedef puntero a función de syscall. */
typedef uint64_t (*syscall_t)(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

/* Inicializa las syscalls */
void initSyscalls();

/* Ejecuta un syscall según syscallID. */
uint64_t syscallDispatcher(uint64_t syscallNumber, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

/* Registra un syscall */
void registerSyscall(uint64_t syscallNumber, syscall_t function);

#endif

