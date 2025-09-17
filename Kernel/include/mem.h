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
 * Syscalls
 */
void *mem_alloc(size_t size);

void mem_free(void *mem);

int mem_check(void *mem);
