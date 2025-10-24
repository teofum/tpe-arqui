#include <mem.h>
#include <spinlock.h>

lock_t lock_create() {
  lock_t lock = mem_alloc(sizeof(int));
  if (lock == NULL) { return -1; }
  *lock = 0;
  return lock;
};

void lock_destroy(lock_t lock) { mem_free(lock); }