#include <mem.h>
#include <stdint.h>

#define MIN_OBJECT_SIZE 32
#define SLAB_SIZE 4096

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
  void *start;
  size_t size;
  uint8_t *bump;
  cache_t *caches;
};

static size_t round_size(size_t size) {
  if (size < MIN_OBJECT_SIZE) return MIN_OBJECT_SIZE;

  size_t rounded = MIN_OBJECT_SIZE;
  while (rounded < size) { rounded <<= 1; }

  return rounded;
}

static void *align_ptr(void *ptr) {
  uint64_t addr = (uint64_t) ptr;
  return (void *) ((addr + SLAB_SIZE - 1) & ~(SLAB_SIZE - 1));
}

static int in_bounds(mem_manager_t mgr, void *ptr) {
  return (uint8_t *) ptr >= (uint8_t *) mgr->start &&
         (uint8_t *) ptr < (uint8_t *) mgr->start + mgr->size;
}

mem_manager_t mem_manager_create(void *mgr_mem, void *mem, size_t mem_size) {
  mem_manager_t mgr = (mem_manager_t) mgr_mem;

  mgr->start = mem;
  mgr->size = mem_size;
  mgr->bump = (uint8_t *) mem;
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
  if (mgr->bump + sizeof(cache_t) > (uint8_t *) mgr->start + mgr->size)
    return NULL;

  cache_t *cache = (cache_t *) mgr->bump;
  mgr->bump += sizeof(cache_t);

  cache->next = mgr->caches;
  cache->object_size = object_size;
  cache->slabs = NULL;

  mgr->caches = cache;
  return cache;
}

static slab_t *create_slab(mem_manager_t mgr, size_t object_size) {
  slab_t *slab = (slab_t *) align_ptr(mgr->bump);

  if ((uint8_t *) slab + SLAB_SIZE > (uint8_t *) mgr->start + mgr->size)
    return NULL;

  uint16_t capacity = (SLAB_SIZE - sizeof(slab_t)) / object_size;

  mgr->bump = (uint8_t *) slab + SLAB_SIZE;

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

  size_t object_size = round_size(size);

  // NOTE: slab cannot handle big objects
  // problem with mem alloc by vga/gfx at init
  // fix?: buddy + slab as in linux
  if (object_size > SLAB_SIZE - sizeof(slab_t)) return NULL;

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

void mem_manager_free(mem_manager_t mgr, void *mem) {
  // TODO
  (void) mgr;
  (void) mem;
}

int mem_manager_check(mem_manager_t mgr, void *mem) {
  // TODO
  (void) mgr;
  (void) mem;
  return 1;
}

void mem_manager_status(
  mem_manager_t mgr, size_t *total, size_t *used, size_t *free
) {
  // TODO
  *total = mgr->size;
  *used = 0;
  *free = mgr->size;
}
