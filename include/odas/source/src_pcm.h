#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm/aml_ringbuffer.h"

#define RB_COUNT 4

#ifdef UAC_SRC_SINK

#include "comm/comm_audio.h"

typedef struct {
  // channel number
  td_u32 chn;
  // channel FD
  td_s32 ai_fd;
  // read frame
  ot_audio_frame frame;
  // read FD
  fd_set read_fds;
} src_pcm_item;

#endif

typedef struct {
  // flag for init
  int init;
  // channel number
  int chn;
  // point num per frame (80/160/240/320/480/1024/2048)
  unsigned int per_frame;
  // frame size
  unsigned int frame_size;

#ifdef UAC_SRC_SINK
  // device for AI
  ot_audio_dev ai_dev;
  // attributes for AI
  ot_aio_attr ai_attr;
  // sample rate
  ot_audio_sample_rate sample_rate;
  // bit width
  ot_audio_bit_width bit_width;

  // for read each channel data
  src_pcm_item items[SAMPLE_AUDIO_AI_CHANNELS];

  // worker thread
  pthread_t worker;
  atomic_bool running;
#endif

  // buffer for read data
  ring_buffer_t *rb;
  char* buffer;

} src_pcm_spec;

int src_pcm_init(src_pcm_spec* obj);
void src_pcm_destory(src_pcm_spec* obj);

#ifdef UAC_SRC_SINK
int src_pcm_read_frame(src_pcm_spec* obj, src_pcm_item* item);
void src_pcm_release_frame(src_pcm_spec* obj, src_pcm_item* item);
#endif

int src_pcm_write_buffer(src_pcm_spec* obj, char** chn_bufs, int chn);
int src_pcm_read_buffer(src_pcm_spec* obj, char* buffer, size_t size);