/*
 * Buddy allocator - splits memory in powers of 2
 * Each order has a list of free blocks of that size
 * When allocating: find block, split if too big
 * When freeing: merge recursively with buddies
 * order 0 = 32 bytes, order 1 = 64 bytes, etc
 */

#include <mem.h>
#include <stdint.h>

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
  block_t *lists[];
};

static uint8_t get_order(size_t size) {
  uint8_t order = 0;
  size_t block_size = MIN_BLOCK_SIZE;

  while (block_size < size) {
    block_size <<= 1;
    order++;
  }

  return order;
}

static int in_bounds(mem_manager_t mgr, void *mem) {
  return (uint8_t *) mem >= (uint8_t *) mgr->start &&
         (uint8_t *) mem < (uint8_t *) mgr->start + mgr->size;
}

static void remove_block(block_t **head, block_t *block) {
  block_t *prev = NULL;
  block_t *curr = *head;

  while (curr && curr != block) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr) return;
  if (prev) {
    prev->next = curr->next;
    return;
  }
  *head = curr->next;
}

/*
 * Find buddy block
 */
static void *get_buddy(void *block, uint8_t order, void *start) {
  // Calculate block size for this order
  size_t block_size = MIN_BLOCK_SIZE << order;
  // Get relative address from start
  size_t addr = (size_t) block - (size_t) start;
  // XOR with block size to find buddy
  return (uint8_t *) start + (addr ^ block_size);
}

mem_manager_t mem_manager_create(void *mgr_mem, void *mem, size_t mem_size) {
  mem_manager_t mgr = (mem_manager_t) mgr_mem;

  mgr->start = mem;
  mgr->size = mem_size;
  mgr->max_order = get_order(mem_size);

  for (uint8_t i = 0; i <= mgr->max_order; i++) { mgr->lists[i] = NULL; }

  block_t *initial = (block_t *) mem;
  initial->next = NULL;
  initial->order = mgr->max_order;
  initial->free = 1;

  mgr->lists[mgr->max_order] = initial;

  return mgr;
}

void *mem_manager_alloc(mem_manager_t mgr, size_t size) {
  if (size == 0 || size > mgr->size) return NULL;

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
  if (mem == NULL) return;

  // Get block header
  block_t *block = (block_t *) ((uint8_t *) mem - sizeof(block_t));

  if (block->free) return;

  block->free = 1;

  // Merge with buddies recursively
  block_t *buddy = (block_t *) get_buddy(block, block->order, mgr->start);
  while (block->order < mgr->max_order && in_bounds(mgr, buddy) &&
         buddy->free && buddy->order == block->order) {
    // Remove buddy from list
    remove_block(&mgr->lists[block->order], buddy);

    // Merge blocks
    if ((uint8_t *) buddy < (uint8_t *) block) { block = buddy; }
    block->order++;

    // Find new buddy
    buddy = (block_t *) get_buddy(block, block->order, mgr->start);
  }

  // Add block to free list
  block->next = mgr->lists[block->order];
  mgr->lists[block->order] = block;
}

int mem_manager_check(mem_manager_t mgr, void *mem) {
  if (mem == NULL) return 0;

  if (!in_bounds(mgr, mem)) { return 0; }

  // Get block header
  block_t *block = (block_t *) ((uint8_t *) mem - sizeof(block_t));

  return !block->free;
}

void mem_manager_status(
  mem_manager_t mgr, size_t *total, size_t *used, size_t *free
) {
  *total = mgr->size;
  *used = 0;
  *free = 0;

  // Count free blocks
  for (uint8_t order = 0; order <= mgr->max_order; order++) {
    block_t *curr = mgr->lists[order];
    while (curr) {
      size_t block_size = MIN_BLOCK_SIZE << order;
      *free += block_size;
      curr = curr->next;
    }
  }

  *used = *total - *free;
}