#include <mem.h>
#include <stdint.h>

struct mem_manager_cdt_t {
  uint8_t *next_address;
};

mem_manager_t mem_manager_create(
  void *manager_address, void *managed_mem_address, size_t managed_mem_size
) {
  mem_manager_t mgr = (mem_manager_t) manager_address;
  mgr->next_address = managed_mem_address;
  return mgr;
}

void *mem_manager_alloc(mem_manager_t mgr, size_t size) {
  void *alloc = mgr->next_address;
  if (size % 8 != 0) size += 8 - (size % 8);// alignment
  mgr->next_address += size;
  return alloc;
}

void mem_manager_free(mem_manager_t mgr, void *mem) {}

int mem_manager_check(mem_manager_t mgr, void *mem) { return 0; }

void mem_manager_status(
  mem_manager_t mgr, size_t *total, size_t *used, size_t *free
) {
  *total = 0;
  *used = 0;
  *free = 0;
}
