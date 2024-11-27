#include "source/src_pcm.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DMIC_GAIN 90 /* 90: 24dB */

static td_void init_params(ot_aio_attr* ai_attr, ot_audio_dev* ai_dev,
                           src_pcm_spec* obj) {
  ai_attr->sample_rate = obj->sample_rate;
  ai_attr->bit_width = obj->bit_width;
  ai_attr->work_mode = OT_AIO_MODE_I2S_MASTER;
  ai_attr->snd_mode = OT_AUDIO_SOUND_MODE_MONO;
  ai_attr->expand_flag = 0;
  ai_attr->frame_num = SAMPLE_AUDIO_AI_USER_FRAME_DEPTH;
  ai_attr->point_num_per_frame = SAMPLE_AUDIO_POINT_NUM_PER_FRAME;
  ai_attr->chn_cnt = obj->chn;

  *ai_dev = SAMPLE_AUDIO_INNER_AI_DEV;
  ai_attr->clk_share = 0;
  ai_attr->i2s_type = OT_AIO_I2STYPE_DMIC;
}

td_s32 src_pcm_init(src_pcm_spec* obj) {
  td_s32 ret;

  // init audio module
  ret = comm_audio_init();
  if (ret != TD_SUCCESS) {
    printf("%s: audio init failed with %d!\n", __FUNCTION__, ret);
    return TD_FAILURE;
  }

  comm_ai_vqe_param ai_vqe_param = {};

  // init audio input params
  init_params(&(obj->ai_attr), &(obj->ai_dev), obj);

  // start ai
  ret = comm_audio_start_ai(obj->ai_dev, obj->chn, &(obj->ai_attr),
                            &ai_vqe_param, -1);
  if (ret != TD_SUCCESS) {
    printf("%s: comm_audio_start_ai failed with %d!\n", __FUNCTION__, ret);
    return ret;
  }

  // set dmic gain
  ret = ss_mpi_ai_set_dmic_gain(obj->ai_dev, DMIC_GAIN);
  if (ret != TD_SUCCESS) {
    printf("%s: ss_mpi_ai_set_dmic_gain failed with %d!\n", __FUNCTION__, ret);
    comm_audio_stop_ai(obj->ai_dev, obj->chn, TD_FALSE, TD_TRUE);
    return ret;
  }

  return TD_SUCCESS;
}

void src_pcm_destory(src_pcm_spec* obj) {
  // stop ai
  comm_audio_stop_ai(obj->ai_dev, obj->chn, TD_FALSE, TD_TRUE);

  // destory module
  comm_audio_exit();
}