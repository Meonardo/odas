#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct {
  // flag for init
  td_bool init;
  // device for AI
  ot_audio_dev ai_dev;
  // attributes for AI
  ot_aio_attr ai_attr;
  // channel number for AI
  td_s32 chn;
  // sample rate
  ot_audio_sample_rate sample_rate;
  // point num per frame (80/160/240/320/480/1024/2048)
  td_u32 per_frame;
  // bit width
  ot_audio_bit_width bit_width;

  // for read each channel data
  src_pcm_item items[SAMPLE_AUDIO_AI_CHANNELS];
} src_pcm_spec;

td_s32 src_pcm_init(src_pcm_spec* obj);
void src_pcm_destory(src_pcm_spec* obj);

td_s32 src_pcm_read_frame(src_pcm_spec* obj, src_pcm_item* item);
void src_pcm_release_frame(src_pcm_spec* obj, src_pcm_item* item);