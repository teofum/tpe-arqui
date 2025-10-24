#ifndef MEM_H
#define MEM_H

#include <stddef.h>

/*
 * Allocate a block of memory using the system allocator. If allocation fails,
 * returns NULL. The allocated memory is guaranteed to be at least size bytes.
 */
void *mem_alloc(size_t size);

/*
 * Free a block of allocated memory from the system allocator.
 * Using freed memory is undefined behavior.
 */
void mem_free(void *mem);

/*
 * Get information on a block of allocated memory.
 * TODO: define this API better
 */
int mem_check(void *mem);

/*
 * Get memory status
 */
void mem_status(size_t *total, size_t *used, size_t *free);

#endif
