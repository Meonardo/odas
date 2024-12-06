#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm/aml_ringbuffer.h"

#define RB_COUNT 4

typedef struct {
  // flag for init
  int init;
  // channel number
  int chn;
  // point num per frame (80/160/240/320/480/1024/2048)
  unsigned int per_frame;
  // frame size
  unsigned int frame_size;

  // buffer for read data
  ring_buffer_t *rb;
  char* buffer;

} src_pcm_spec;

int src_pcm_init(src_pcm_spec* obj);
void src_pcm_destory(src_pcm_spec* obj);

int src_pcm_write_buffer(src_pcm_spec* obj, char** chn_bufs, int chn);
int src_pcm_read_buffer(src_pcm_spec* obj, char* buffer, size_t size);