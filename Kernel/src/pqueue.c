#include <mem.h>
#include <pqueue.h>
#include <process.h>
#include <scheduler.h>
#include <stdint.h>
#include <types.h>

typedef struct pqueue_item_t {
  pid_t data;
  struct pqueue_item_t *tail;
} pqueue_item_t;

struct pqueue_cdt_t {
  pqueue_item_t *head;
};

pqueue_t pqueue_create() {
  pqueue_t queue = mem_alloc(sizeof(struct pqueue_cdt_t));
  queue->head = NULL;

  return queue;
}

static pqueue_item_t *pqueue_create_item(pid_t pid) {
  pqueue_item_t *item = mem_alloc(sizeof(pqueue_item_t));
  item->data = pid;
  item->tail = NULL;

  return item;
}

static void pqueue_add_after(pqueue_item_t *item, pqueue_item_t *new_item) {
  if (item->tail == NULL) {
    item->tail = new_item;
  } else {
    pqueue_add_after(item->tail, new_item);
  }
}

void pqueue_enqueue(pqueue_t queue, pid_t pid) {
  pqueue_item_t *item = pqueue_create_item(pid);

  if (pqueue_empty(queue)) {
    queue->head = item;
  } else {
    pqueue_add_after(queue->head, item);
  }
}

pid_t pqueue_dequeue(pqueue_t queue) {
  if (pqueue_empty(queue)) return -1;

  pqueue_item_t *head = queue->head;
  pid_t pid = head->data;
  queue->head = head->tail;
  mem_free(head);

  return pid;
}

pid_t pqueue_dequeue_and_run(pqueue_t queue) {
  pid_t pid = pqueue_dequeue(queue);

  if (pid != -1) {
    proc_control_block_t *waiting_pcb = &proc_control_table[pid];

    if (waiting_pcb->state == PROC_STATE_BLOCKED) {
      waiting_pcb->state = PROC_STATE_RUNNING;
      scheduler_enqueue(pid);
    }
  }

  return pid;
}

static int pqueue_has_impl(pqueue_item_t *item, pid_t pid) {
  if (item->data == pid) return 1;
  if (item->tail == NULL) return 0;
  return pqueue_has_impl(item->tail, pid);
}

int pqueue_has(pqueue_t queue, pid_t pid) {
  if (pqueue_empty(queue)) return 0;
  return pqueue_has_impl(queue->head, pid);
}

int pqueue_empty(pqueue_t queue) { return queue->head == NULL; }

static pqueue_item_t *
pqueue_remove_impl(pqueue_item_t *item, pid_t pid, uint32_t *count) {
  if (item == NULL) return item;

  pqueue_item_t *tail = pqueue_remove_impl(item->tail, pid, count);
  if (item->data == pid) {
    (*count)++;
    mem_free(item);
    return tail;
  } else {
    item->tail = tail;
    return item;
  }
}

uint32_t pqueue_remove_all(pqueue_t queue, pid_t pid) {
  if (pqueue_empty(queue)) return 0;

  uint32_t removed = 0;
  queue->head = pqueue_remove_impl(queue->head, pid, &removed);
  return removed;
}

static void pqueue_destroy_impl(pqueue_item_t *item) {
  if (item->tail) pqueue_destroy_impl(item->tail);
  mem_free(item);
}

void pqueue_destroy(pqueue_t queue) {
  if (queue->head) pqueue_destroy_impl(queue->head);
  mem_free(queue);
}
