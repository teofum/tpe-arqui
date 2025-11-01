/*
 * Buddy allocator - splits memory in powers of 2
 * Each order has a list of free blocks of that size
 * When allocating: find block, split if too big
 * When freeing: merge recursively with buddies
 * order 0 = 32 bytes, order 1 = 64 bytes, etc
 */

#include <buddy.h>
#include <stdint.h>

#define MIN_BLOCK_SIZE 32

typedef struct block {
  struct block *next;
  uint8_t order;
  uint8_t free;
} block_t;

struct buddy_allocator {
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

static int in_bounds(buddy_allocator_t *buddy, void *mem) {
  return (uint8_t *) mem >= (uint8_t *) buddy->start &&
         (uint8_t *) mem < (uint8_t *) buddy->start + buddy->size;
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

buddy_allocator_t *buddy_create(void *buddy_mem, void *mem, size_t mem_size) {
  buddy_allocator_t *buddy = (buddy_allocator_t *) buddy_mem;

  buddy->start = mem;
  buddy->size = mem_size;
  buddy->max_order = get_order(mem_size);

  for (uint8_t i = 0; i <= buddy->max_order; i++) { buddy->lists[i] = NULL; }

  block_t *initial = (block_t *) mem;
  initial->next = NULL;
  initial->order = buddy->max_order;
  initial->free = 1;

  buddy->lists[buddy->max_order] = initial;

  return buddy;
}

void *buddy_alloc(buddy_allocator_t *buddy, size_t size) {
  if (size == 0 || size > buddy->size) return NULL;

  uint8_t order = get_order(size + sizeof(block_t));

  uint8_t curr = order;
  while (curr <= buddy->max_order && buddy->lists[curr] == NULL) { curr++; }

  if (curr > buddy->max_order) return NULL;

  block_t *block = buddy->lists[curr];
  buddy->lists[curr] = block->next;

  while (curr > order) {
    curr--;

    size_t block_size = MIN_BLOCK_SIZE << curr;
    block_t *buddy_block = (block_t *) ((uint8_t *) block + block_size);

    buddy_block->order = curr;
    buddy_block->free = 1;
    buddy_block->next = buddy->lists[curr];
    buddy->lists[curr] = buddy_block;
  }

  block->free = 0;
  block->order = order;

  return (uint8_t *) block + sizeof(block_t);
}

void buddy_free(buddy_allocator_t *buddy, void *mem) {
  if (mem == NULL) return;

  block_t *block = (block_t *) ((uint8_t *) mem - sizeof(block_t));

  if (block->free) return;

  block->free = 1;

  block_t *buddy_block = (block_t *) get_buddy(block, block->order, buddy->start);
  while (block->order < buddy->max_order && in_bounds(buddy, buddy_block) &&
         buddy_block->free && buddy_block->order == block->order) {
    remove_block(&buddy->lists[block->order], buddy_block);

    if ((uint8_t *) buddy_block < (uint8_t *) block) block = buddy_block;
    block->order++;

    buddy_block = (block_t *) get_buddy(block, block->order, buddy->start);
  }

  block->next = buddy->lists[block->order];
  buddy->lists[block->order] = block;
}

int buddy_check(buddy_allocator_t *buddy, void *mem) {
  if (mem == NULL) return 0;

  if (!in_bounds(buddy, mem)) return 0;

  block_t *block = (block_t *) ((uint8_t *) mem - sizeof(block_t));

  return !block->free;
}

void buddy_status(
  buddy_allocator_t *buddy, size_t *total, size_t *used, size_t *free
) {
  *total = buddy->size;
  *used = 0;
  *free = 0;

  for (uint8_t order = 0; order <= buddy->max_order; order++) {
    block_t *curr = buddy->lists[order];
    while (curr) {
      size_t block_size = MIN_BLOCK_SIZE << order;
      *free += block_size;
      curr = curr->next;
    }
  }

  *used = *total - *free;
}

