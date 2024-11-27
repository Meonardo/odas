#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm/comm_audio.h"

typedef struct {
  td_u32 chn;
  td_s32 ai_fd;
  ot_audio_frame frame;
  ot_aec_frame aec_frm;
  fd_set read_fds;
} src_pcm_item;

typedef struct {
  td_bool start;
  ot_audio_dev ai_dev;
  ot_aio_attr ai_attr;
  td_s32 chn;
  ot_audio_sample_rate sample_rate;
  td_u32 per_frame;
  ot_audio_bit_width bit_width;

  src_pcm_item items[SAMPLE_AUDIO_AI_CHANNELS];
} src_pcm_spec;

td_s32 src_pcm_init(src_pcm_spec* obj);
void src_pcm_destory(src_pcm_spec* obj);

td_s32 src_pcm_read_frame(src_pcm_spec* obj, src_pcm_item* item);
void src_pcm_release_frame(src_pcm_spec* obj, src_pcm_item* item);