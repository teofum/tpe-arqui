#ifndef MEM_H
#define MEM_H

#include <stddef.h>

typedef struct mem_manager_cdt_t *mem_manager_t;

extern mem_manager_t mem_default_mgr;

mem_manager_t mem_manager_create(
  void *manager_address, void *managed_mem_address, size_t managed_mem_size
);

void *mem_manager_alloc(mem_manager_t mgr, size_t size);

void mem_manager_free(mem_manager_t mgr, void *mem);

int mem_manager_check(mem_manager_t mgr, void *mem);

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
