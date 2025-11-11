#ifndef PQUEUE_H
#define PQUEUE_H

#include <types.h>

typedef struct pqueue_cdt_t *pqueue_t;

pqueue_t pqueue_create();

void pqueue_enqueue(pqueue_t queue, pid_t pid);

pid_t pqueue_dequeue(pqueue_t queue);

pid_t pqueue_dequeue_and_run(pqueue_t queue);

int pqueue_has(pqueue_t queue, pid_t pid);

int pqueue_empty(pqueue_t queue);

uint32_t pqueue_remove_all(pqueue_t queue, pid_t pid);

void pqueue_destroy(pqueue_t queue);

#endif
