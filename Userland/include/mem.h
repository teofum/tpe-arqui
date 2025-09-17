#include <stddef.h>

/*
 * Syscalls
 */
void *mem_alloc(size_t size);

void mem_free(void *mem);

int mem_check(void *mem);
