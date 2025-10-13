#include <mem.h>
#include <stdint.h>

mem_manager_t mem_default_mgr = NULL;

void *mem_alloc(size_t size) {
  return mem_manager_alloc(mem_default_mgr, size);
}

void mem_free(void *mem) { mem_manager_free(mem_default_mgr, mem); }

int mem_check(void *mem) { return mem_manager_check(mem_default_mgr, mem); }

void mem_status(size_t *total, size_t *used, size_t *free) {
  mem_manager_status(mem_default_mgr, total, used, free);
}
