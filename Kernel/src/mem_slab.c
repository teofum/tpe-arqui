#include <mem.h>
#include <buddy.h>
#include <stdint.h>

#define MIN_OBJECT_SIZE 32
#define SLAB_PAGE_SIZE 4096
#define SLUB_THRESHOLD 4096

typedef struct free_node {
  struct free_node *next;
} free_node_t;

typedef struct slab {
  struct slab *next;
  uint16_t free_count;
  free_node_t *free_list;
} slab_t;

typedef struct cache {
  struct cache *next;
  size_t object_size;
  slab_t *slabs;
} cache_t;

struct mem_manager_cdt_t {
  buddy_allocator_t *buddy;
  cache_t *caches;
};

static size_t round_size(size_t size) {
  if (size < MIN_OBJECT_SIZE) return MIN_OBJECT_SIZE;

  size_t rounded = MIN_OBJECT_SIZE;
  while (rounded < size) { rounded <<= 1; }

  return rounded;
}

mem_manager_t mem_manager_create(void *mgr_mem, void *mem, size_t mem_size) {
  mem_manager_t mgr = (mem_manager_t) mgr_mem;

  mgr->buddy = buddy_create(mgr_mem + sizeof(struct mem_manager_cdt_t), mem, mem_size);
  mgr->caches = NULL;

  return mgr;
}

static cache_t *find_cache(mem_manager_t mgr, size_t object_size) {
  cache_t *cache = mgr->caches;
  while (cache) {
    if (cache->object_size == object_size) return cache;
    cache = cache->next;
  }
  return NULL;
}

static cache_t *create_cache(mem_manager_t mgr, size_t object_size) {
  cache_t *cache = buddy_alloc(mgr->buddy, sizeof(cache_t));
  if (!cache) return NULL;

  cache->next = mgr->caches;
  cache->object_size = object_size;
  cache->slabs = NULL;

  mgr->caches = cache;
  return cache;
}

static slab_t *create_slab(mem_manager_t mgr, size_t object_size) {
  slab_t *slab = buddy_alloc(mgr->buddy, SLAB_PAGE_SIZE);
  if (!slab) return NULL;

  uint16_t capacity = (SLAB_PAGE_SIZE - sizeof(slab_t)) / object_size;

  slab->next = NULL;
  slab->free_count = capacity;
  slab->free_list = NULL;

  uint8_t *obj = (uint8_t *) slab + sizeof(slab_t);
  for (uint16_t i = 0; i < capacity; i++) {
    free_node_t *node = (free_node_t *) (obj + i * object_size);
    node->next = slab->free_list;
    slab->free_list = node;
  }

  return slab;
}

void *mem_manager_alloc(mem_manager_t mgr, size_t size) {
  if (size == 0) return NULL;

  if (size >= SLUB_THRESHOLD) {
    return buddy_alloc(mgr->buddy, size);
  }

  size_t object_size = round_size(size);

  cache_t *cache = find_cache(mgr, object_size);
  if (!cache) {
    cache = create_cache(mgr, object_size);
    if (!cache) return NULL;
  }

  slab_t *slab = cache->slabs;
  while (slab && slab->free_count == 0) { slab = slab->next; }

  if (!slab) {
    slab = create_slab(mgr, object_size);
    if (!slab) return NULL;

    slab->next = cache->slabs;
    cache->slabs = slab;
  }

  free_node_t *node = slab->free_list;
  slab->free_list = node->next;
  slab->free_count--;

  return node;
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