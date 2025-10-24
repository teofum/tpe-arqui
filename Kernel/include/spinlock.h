#ifndef SPINLOCK_H
#define SPINLOCK_H


typedef int *lock_t;

lock_t lock_create();

void lock_acquire(lock_t lock);

void lock_release(lock_t lock);

void lock_destroy(lock_t lock);

#endif