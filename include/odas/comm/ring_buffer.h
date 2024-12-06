#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint8_t *buffer;    /* Buffer memory */
  size_t size;        /* Size of the buffer (must be a power of two) */
  atomic_size_t head; /* Write pointer (producer index) */
  atomic_size_t tail; /* Read pointer (consumer index) */
} ring_buffer_t;

/* Initialize the ring buffer */
int ring_buffer_init(ring_buffer_t *rb, size_t size);

/* Free the ring buffer */
void ring_buffer_free(ring_buffer_t *rb);

/* Get the capacity of the ring buffer */
size_t ring_buffer_capacity(ring_buffer_t *rb);

/* Get the number of bytes stored in the ring buffer */
size_t ring_buffer_size(ring_buffer_t *rb);

/* Check if the ring buffer is empty */
int ring_buffer_empty(ring_buffer_t *rb);

/* Check if the ring buffer is full */
int ring_buffer_full(ring_buffer_t *rb);

/* Write data to the ring buffer (Producer) */
size_t ring_buffer_write(ring_buffer_t *rb, const uint8_t *data, size_t bytes);

/* Read data from the ring buffer (Consumer) */
size_t ring_buffer_read(ring_buffer_t *rb, uint8_t *data, size_t bytes);
