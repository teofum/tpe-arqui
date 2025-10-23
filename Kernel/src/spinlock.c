#include <mem.h>
#include <spinlock.h>

lock_t lock_create() { return mem_alloc(sizeof(int)); };

void lock_destroy(lock_t lock) { mem_free(lock); }