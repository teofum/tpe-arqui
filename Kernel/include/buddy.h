#ifndef BUDDY_H
#define BUDDY_H

#include <stddef.h>

typedef struct buddy_allocator buddy_allocator_t;

buddy_allocator_t *buddy_create(void *buddy_mem, void *mem, size_t mem_size);

void *buddy_alloc(buddy_allocator_t *buddy, size_t size);

void buddy_free(buddy_allocator_t *buddy, void *mem);

int buddy_check(buddy_allocator_t *buddy, void *mem);

void buddy_status(
  buddy_allocator_t *buddy, size_t *total, size_t *used, size_t *free
);

#endif
