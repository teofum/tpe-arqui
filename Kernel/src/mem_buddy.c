/*
 * Buddy allocator - splits memory in powers of 2
 * Each order has a list of free blocks of that size
 * When allocating: find block, split if too big
 * When freeing: merge with buddy if both free
 * order 0 = 32 bytes, order 1 = 64 bytes, etc
 */

#include <mem.h>
#include <stdint.h>

#define MAX_ORDER 20
#define MIN_BLOCK_SIZE 32

typedef struct block {
  struct block *next;
  uint8_t order;
  uint8_t free;
} block_t;

struct mem_manager_cdt_t {
  void *start;
  size_t size;
  uint8_t max_order;
  block_t *lists[MAX_ORDER];
};

static uint8_t get_order(size_t size) {
  uint8_t order = 0;
  size_t block_size = MIN_BLOCK_SIZE;

  while (block_size < size && order < MAX_ORDER) {
    block_size <<= 1;
    order++;
  }

  return order;
}

mem_manager_t mem_manager_create(void *mgr_mem, void *mem, size_t mem_size) {
  mem_manager_t mgr = (mem_manager_t) mgr_mem;

  mgr->start = mem;
  mgr->size = mem_size;
  mgr->max_order = get_order(mem_size);

  for (uint8_t i = 0; i < MAX_ORDER; i++) { mgr->lists[i] = NULL; }

  block_t *initial = (block_t *) mem;
  initial->next = NULL;
  initial->order = mgr->max_order;
  initial->free = 1;

  mgr->lists[mgr->max_order] = initial;

  return mgr;
}

void *mem_manager_alloc(mem_manager_t mgr, size_t size) {
  if (size == 0) return NULL;

  uint8_t order = get_order(size + sizeof(block_t));

  // Find free block of required order or more
  uint8_t curr = order;
  while (curr <= mgr->max_order && mgr->lists[curr] == NULL) { curr++; }

  if (curr > mgr->max_order) { return NULL; }

  block_t *block = mgr->lists[curr];
  mgr->lists[curr] = block->next;

  // Split block to required order
  while (curr > order) {
    curr--;

    size_t block_size = MIN_BLOCK_SIZE << curr;
    block_t *buddy = (block_t *) ((uint8_t *) block + block_size);

    buddy->order = curr;
    buddy->free = 1;
    buddy->next = mgr->lists[curr];
    mgr->lists[curr] = buddy;
  }

  block->free = 0;
  block->order = order;

  return (uint8_t *) block + sizeof(block_t);
}

void mem_manager_free(mem_manager_t mgr, void *mem) {
  // TODO
}

int mem_manager_check(mem_manager_t mgr, void *mem) {
  return 0; // TODO
}