/*
 * Bitmap allocator - fixed 64-byte blocks
 * Each bit represents one block: 1 = occupied, 0 = free
 */

#include <mem.h>
#include <stdint.h>

#define BLOCK_SIZE 64
#define BITS_PER_BYTE 8// silly but makes code more understandable?

typedef struct {
  size_t block_count;
} alloc_header_t;

struct mem_manager_cdt_t {
  void *start;
  size_t size;
  size_t total_blocks;
  uint8_t *bitmap;
};

static void set_bit(uint8_t *bitmap, size_t index) {
  bitmap[index / BITS_PER_BYTE] |= (1 << (index % BITS_PER_BYTE));
}

static void clear_bit(uint8_t *bitmap, size_t index) {
  bitmap[index / BITS_PER_BYTE] &= ~(1 << (index % BITS_PER_BYTE));
}

static int get_bit(uint8_t *bitmap, size_t index) {
  return (bitmap[index / BITS_PER_BYTE] >> (index % BITS_PER_BYTE)) & 1;
}

static int in_bounds(mem_manager_t mgr, void *mem) {
  return (uint8_t *) mem >= (uint8_t *) mgr->start &&
         (uint8_t *) mem < (uint8_t *) mgr->start + mgr->size;
}

static void *find_free_blocks(mem_manager_t mgr, size_t needed) {
  if (needed > mgr->total_blocks) return NULL;

  size_t bitmap_bytes = (mgr->total_blocks + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

  for (size_t byte = 0; byte < bitmap_bytes; byte++) {
    // skip bytes with all blocks occupied
    if (mgr->bitmap[byte] != 0xFF) {
      for (size_t bit = 0; bit < BITS_PER_BYTE; bit++) {
        size_t i = byte * BITS_PER_BYTE + bit;
        if (i > mgr->total_blocks - needed) return NULL;

        // check for consecutive free blocks
        size_t j;
        for (j = 0; j < needed; j++) {
          if (get_bit(mgr->bitmap, i + j)) break;
        }
        if (j == needed) { return (uint8_t *) mgr->start + i * BLOCK_SIZE; }
      }
    }
  }
  return NULL;
}

mem_manager_t mem_manager_create(
  void *manager_address, void *managed_mem_address, size_t managed_mem_size
) {
  mem_manager_t mgr = (mem_manager_t) manager_address;

  size_t total_blocks = managed_mem_size / BLOCK_SIZE;
  size_t bitmap_bytes = (total_blocks + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
  size_t bitmap_blocks = (bitmap_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;

  mgr->bitmap = (uint8_t *) managed_mem_address;
  mgr->start = (uint8_t *) managed_mem_address + bitmap_blocks * BLOCK_SIZE;
  mgr->size = managed_mem_size - bitmap_blocks * BLOCK_SIZE;
  mgr->total_blocks = mgr->size / BLOCK_SIZE;

  for (size_t i = 0; i < bitmap_bytes; i++) { mgr->bitmap[i] = 0; }

  return mgr;
}

void *mem_manager_alloc(mem_manager_t mgr, size_t size) {
  if (size == 0 || size > mgr->size) return NULL;

  size_t blocks_needed =
    (size + sizeof(alloc_header_t) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
  alloc_header_t *alloc =
    (alloc_header_t *) find_free_blocks(mgr, blocks_needed);

  if (!alloc) return NULL;

  size_t start = ((uint8_t *) alloc - (uint8_t *) mgr->start) / BLOCK_SIZE;
  for (size_t i = 0; i < blocks_needed; i++) {
    set_bit(mgr->bitmap, start + i);
  }
  alloc->block_count = blocks_needed;

  return (uint8_t *) alloc + sizeof(alloc_header_t);
}

void mem_manager_free(mem_manager_t mgr, void *mem) {
  if (mem == NULL) return;

  alloc_header_t *alloc =
    (alloc_header_t *) ((uint8_t *) mem - sizeof(alloc_header_t));

  if (!in_bounds(mgr, alloc)) return;

  size_t start = ((uint8_t *) alloc - (uint8_t *) mgr->start) / BLOCK_SIZE;

  if (start + alloc->block_count > mgr->total_blocks) return;

  if (!get_bit(mgr->bitmap, start)) return;

  for (size_t i = 0; i < alloc->block_count; i++) {
    clear_bit(mgr->bitmap, start + i);
  }
}

int mem_manager_check(mem_manager_t mgr, void *mem) {
  if (mem == NULL) return 0;

  if (!in_bounds(mgr, mem)) return 0;

  alloc_header_t *alloc =
    (alloc_header_t *) ((uint8_t *) mem - sizeof(alloc_header_t));
  size_t start = ((uint8_t *) alloc - (uint8_t *) mgr->start) / BLOCK_SIZE;

  return get_bit(mgr->bitmap, start);
}

void mem_manager_status(
  mem_manager_t mgr, size_t *total, size_t *used, size_t *free
) {
  size_t used_blocks = 0;
  for (size_t i = 0; i < mgr->total_blocks; i++) {
    if (get_bit(mgr->bitmap, i)) used_blocks++;
  }

  *total = mgr->size;
  *used = used_blocks * BLOCK_SIZE;
  *free = *total - *used;
}