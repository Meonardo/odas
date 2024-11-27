#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm/comm_audio.h"

typedef struct {
  td_bool start;
  ot_audio_dev ai_dev;
  ot_aio_attr ai_attr;
  td_s32 chn;
  ot_audio_sample_rate sample_rate;
  td_u32 per_frame;
  ot_audio_bit_width bit_width;
} src_pcm_spec;

td_s32 src_pcm_init(src_pcm_spec* obj);
void src_pcm_destory(src_pcm_spec* obj);