
#include "comm/ring_buffer.h"

/* Check if a number is a power of two */
#define IS_POWER_OF_TWO(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

/* Initialize the ring buffer */
int ring_buffer_init(ring_buffer_t *rb, size_t size) {
  if (!IS_POWER_OF_TWO(size)) {
    return -1; /* Size must be a power of two */
  }
  rb->buffer = (uint8_t *)malloc(size);
  if (!rb->buffer) {
    return -1; /* Memory allocation failed */
  }
  rb->size = size;
  atomic_init(&rb->head, 0);
  atomic_init(&rb->tail, 0);
  return 0;
}

/* Free the ring buffer */
void ring_buffer_free(ring_buffer_t *rb) { free(rb->buffer); }

/* Get the capacity of the ring buffer */
size_t ring_buffer_capacity(ring_buffer_t *rb) { return rb->size - 1; }

/* Get the number of bytes stored in the ring buffer */
size_t ring_buffer_size(ring_buffer_t *rb) {
  size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
  size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
  return (head - tail) & (rb->size - 1);
}

/* Check if the ring buffer is empty */
int ring_buffer_empty(ring_buffer_t *rb) { return ring_buffer_size(rb) == 0; }

/* Check if the ring buffer is full */
int ring_buffer_full(ring_buffer_t *rb) {
  return ring_buffer_size(rb) == ring_buffer_capacity(rb);
}

/* Write data to the ring buffer (Producer) */
size_t ring_buffer_write(ring_buffer_t *rb, const uint8_t *data, size_t bytes) {
  size_t capacity = rb->size;
  size_t head = atomic_load_explicit(
      &rb->head, memory_order_relaxed);  // Only updated by producer
  size_t tail = atomic_load_explicit(
      &rb->tail, memory_order_acquire);  // Read latest tail value

  size_t free_space = (tail + capacity - head - 1) & (capacity - 1);

  if (bytes > free_space) {
    bytes = free_space; /* Write only what can fit */
  }

  size_t head_index = head & (capacity - 1);
  size_t first_chunk = capacity - head_index;

  if (bytes > first_chunk) {
    memcpy(rb->buffer + head_index, data, first_chunk);
    memcpy(rb->buffer, data + first_chunk, bytes - first_chunk);
  } else {
    memcpy(rb->buffer + head_index, data, bytes);
  }

  /* Ensure that data is written before updating the head pointer */
  atomic_thread_fence(memory_order_release);

  atomic_store_explicit(&rb->head, head + bytes, memory_order_release);

  return bytes;
}

/* Read data from the ring buffer (Consumer) */
size_t ring_buffer_read(ring_buffer_t *rb, uint8_t *data, size_t bytes) {
  size_t capacity = rb->size;
  size_t tail = atomic_load_explicit(
      &rb->tail, memory_order_relaxed);  // Only updated by consumer
  size_t head = atomic_load_explicit(
      &rb->head, memory_order_acquire);  // Read latest head value

  size_t available_data = (head - tail) & (capacity - 1);

  if (bytes > available_data) {
    bytes = available_data; /* Read only what's available */
  }

  size_t tail_index = tail & (capacity - 1);
  size_t first_chunk = capacity - tail_index;

  if (bytes > first_chunk) {
    memcpy(data, rb->buffer + tail_index, first_chunk);
    memcpy(data + first_chunk, rb->buffer, bytes - first_chunk);
  } else {
    memcpy(data, rb->buffer + tail_index, bytes);
  }

  /* Ensure that data is read before updating the tail pointer */
  atomic_thread_fence(memory_order_release);

  atomic_store_explicit(&rb->tail, tail + bytes, memory_order_release);

  return bytes;
}