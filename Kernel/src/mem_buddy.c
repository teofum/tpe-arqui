#include <mem.h>
#include <buddy.h>

mem_manager_t mem_manager_create(
  void *mgr_mem, void *mem, size_t mem_size
) {
  return (mem_manager_t) buddy_create(mgr_mem, mem, mem_size);
}

void *mem_manager_alloc(mem_manager_t mgr, size_t size) {
  return buddy_alloc((buddy_allocator_t *) mgr, size);
}

void mem_manager_free(mem_manager_t mgr, void *mem) {
  buddy_free((buddy_allocator_t *) mgr, mem);
}

int mem_manager_check(mem_manager_t mgr, void *mem) {
  return buddy_check((buddy_allocator_t *) mgr, mem);
}

void mem_manager_status(
  mem_manager_t mgr, size_t *total, size_t *used, size_t *free
) {
  buddy_status((buddy_allocator_t *) mgr, total, used, free);
}