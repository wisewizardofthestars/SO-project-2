#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "producer-consumer.h"
#include "logging.h"


// pc_queue_t queue;

int pcq_create(pc_queue_t *queue, size_t capacity) {
  // Allocate memory for the queue buffer
  queue->pcq_buffer = (void **) malloc(capacity * sizeof(void *));
  if (queue->pcq_buffer == NULL) {
    return -1;
  }

  queue->pcq_capacity = capacity;

  // Initialize the locks and condition variables
  pthread_mutex_init(&queue->pcq_current_size_lock, NULL);
  queue->pcq_current_size = 0;

  pthread_mutex_init(&queue->pcq_head_lock, NULL);
  queue->pcq_head = 0;

  pthread_mutex_init(&queue->pcq_tail_lock, NULL);
  queue->pcq_tail = 0;

  pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL);
  pthread_cond_init(&queue->pcq_pusher_condvar, NULL);

  pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL);
  pthread_cond_init(&queue->pcq_popper_condvar, NULL);

  return 0;
}

int pcq_destroy(pc_queue_t *queue) {
  // Free the queue buffer
  free(queue->pcq_buffer);

  // Destroy the locks and condition variables
  pthread_mutex_destroy(&queue->pcq_current_size_lock);
  pthread_mutex_destroy(&queue->pcq_head_lock);
  pthread_mutex_destroy(&queue->pcq_tail_lock);
  pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
  pthread_cond_destroy(&queue->pcq_pusher_condvar);
  pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
  pthread_cond_destroy(&queue->pcq_popper_condvar);

  return 0;
}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
  // Lock the current size lock
  pthread_mutex_lock(&queue->pcq_current_size_lock);

  // Wait until the queue has space
  while (queue->pcq_current_size == queue->pcq_capacity) {
    pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_current_size_lock);
  }

  // Lock the head lock
  pthread_mutex_lock(&queue->pcq_head_lock);

  // Insert the element at the head of the queue
  queue->pcq_buffer[queue->pcq_head] = elem;
  queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
  queue->pcq_current_size++;

  // Unlock the head lock
  pthread_mutex_unlock(&queue->pcq_head_lock);
  pthread_mutex_unlock(&queue->pcq_current_size_lock);

  pthread_mutex_lock(&queue->pcq_popper_condvar_lock);
  pthread_cond_signal(&queue->pcq_popper_condvar);
  pthread_mutex_unlock(&queue->pcq_popper_condvar_lock);

  return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {
  // Lock the current size lock
  pthread_mutex_lock(&queue->pcq_current_size_lock);

  // Wait until the queue has an element
  while (queue->pcq_current_size == 0) {
    pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_current_size_lock);
  }

  // Lock the tail lock
  pthread_mutex_lock(&queue->pcq_tail_lock);

  // Remove the element at the tail of the queue
  void *elem = queue->pcq_buffer[queue->pcq_tail];
  queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;
  queue->pcq_current_size--;

  pthread_mutex_unlock(&queue->pcq_tail_lock);
  pthread_mutex_unlock(&queue->pcq_current_size_lock);
  pthread_mutex_lock(&queue->pcq_pusher_condvar_lock);
  pthread_cond_signal(&queue->pcq_pusher_condvar);
  pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock);

  return elem;
}