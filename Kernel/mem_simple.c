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
  mgr->next_address += size;
  return alloc;
}

void mem_manager_free(mem_manager_t mgr, void *mem) {}

int mem_manager_check(mem_manager_t mgr, void *mem) { return 0; }
