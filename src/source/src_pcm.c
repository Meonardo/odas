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
  ai_attr->point_num_per_frame = obj->per_frame;
  ai_attr->chn_cnt = obj->chn;

  *ai_dev = SAMPLE_AUDIO_INNER_AI_DEV;
  ai_attr->clk_share = 0;
  ai_attr->i2s_type = OT_AIO_I2STYPE_DMIC;
}

static td_s32 app_audio_select(td_s32 ai_fd, fd_set* read_fds) {
  td_s32 ret;
  struct timeval time_out_val;
  time_out_val.tv_sec = 1;
  time_out_val.tv_usec = 0;

  FD_ZERO(read_fds);
  FD_SET(ai_fd, read_fds);

  ret = select(ai_fd + 1, read_fds, TD_NULL, TD_NULL, &time_out_val);
  if (ret < 0) {
    return TD_FAILURE;
  } else if (ret == 0) {
    printf("[%s] get ai frame select time out\n", __FUNCTION__);
    return TD_FAILURE;
  }
  return TD_SUCCESS;
}

td_s32 src_pcm_init(src_pcm_spec* obj) {
  td_s32 ret;

  // init audio module
  ret = comm_audio_init();
  if (ret != TD_SUCCESS) {
    printf("[%s] audio init failed with %d!\n", __FUNCTION__, ret);
    return TD_FAILURE;
  }

  comm_ai_vqe_param ai_vqe_param = {};

  // init audio input params
  init_params(&(obj->ai_attr), &(obj->ai_dev), obj);

  // start ai
  ret = comm_audio_start_ai(obj->ai_dev, obj->chn, &obj->ai_attr,
                            &ai_vqe_param, -1);
  if (ret != TD_SUCCESS) {
    printf("[%s] comm_audio_start_ai failed with %d!\n", __FUNCTION__, ret);
    return ret;
  }

  // set dmic gain
  ret = ss_mpi_ai_set_dmic_gain(obj->ai_dev, DMIC_GAIN);
  if (ret != TD_SUCCESS) {
    printf("[%s] ss_mpi_ai_set_dmic_gain failed with %d!\n", __FUNCTION__, ret);
    comm_audio_stop_ai(obj->ai_dev, obj->chn, TD_FALSE, TD_TRUE);
    return ret;
  }

  // init items
  for (int i = 0; i < obj->chn; i++) {
    obj->items[i].chn = i;
    obj->items[i].ai_fd = -1;

    ot_ai_chn_param ai_chn_para;
    ret = ss_mpi_ai_get_chn_param(obj->ai_dev, i, &ai_chn_para);
    if (ret != TD_SUCCESS) {
      printf("%s: get ai chn param failed\n", __FUNCTION__);
      return TD_FAILURE;
    }
    ai_chn_para.usr_frame_depth = SAMPLE_AUDIO_AI_USER_FRAME_DEPTH;
    ret = ss_mpi_ai_set_chn_param(obj->ai_dev, i, &ai_chn_para);
    if (ret != TD_SUCCESS) {
      printf("%s: set ai chn param failed, ret=0x%x\n", __FUNCTION__, ret);
      return TD_FAILURE;
    }

    td_s32 ai_fd = ss_mpi_ai_get_fd(obj->ai_dev, i);
    if (ai_fd < 0) {
      printf("%s: get ai fd failed\n", __FUNCTION__);
      return TD_FAILURE;
    }

    obj->items[i].ai_fd = ai_fd;

    printf("[%s] ai(%d,%d) fd = %d\n", __FUNCTION__, obj->ai_dev, i, ai_fd);
  }

  printf("[%s] src_pcm_spec init success!\n\n\n", __FUNCTION__);

  return TD_SUCCESS;
}

void src_pcm_destory(src_pcm_spec* obj) {
  // stop ai
  comm_audio_stop_ai(obj->ai_dev, obj->chn, TD_FALSE, TD_TRUE);

  // destory module
  comm_audio_exit();

  printf("[%s] src_pcm_spec destory success!\n\n\n", __FUNCTION__);
}

td_s32 src_pcm_read_frame(src_pcm_spec* obj, src_pcm_item* item) {
  td_s32 ret;

  ret = app_audio_select(item->ai_fd, &item->read_fds);
  if (ret != TD_SUCCESS) {
    printf("%s: app_audio_select failed with %d!\n", __FUNCTION__, ret);
    return TD_FAILURE;
  }

  if (FD_ISSET(item->ai_fd, &(item->read_fds))) {
    /* get frame from ai chn */
    ret =
        ss_mpi_ai_get_frame(obj->ai_dev, item->chn, &item->frame, NULL, TD_FALSE);
    if (ret != TD_SUCCESS) {
      printf("[%s] ss_mpi_ai_get_frame failed with %d!\n", __FUNCTION__, ret);
      return TD_FAILURE;
    }

    // printf("[%s] ai(%d,%d) get frame, len = %d\n", __FUNCTION__, obj->ai_dev, item->chn,
    //        item->frame.len);
  }

  return TD_SUCCESS;
}

void src_pcm_release_frame(src_pcm_spec* obj, src_pcm_item* item) {
  td_s32 ret =
      ss_mpi_ai_release_frame(obj->ai_dev, item->chn, &item->frame, NULL);
  if (ret != TD_SUCCESS) {
    printf("[%s] ss_mpi_ai_release_frame(%d, %d), failed with %#x!\n",
           __FUNCTION__, obj->ai_dev, item->chn, ret);
    return;
  }

  // printf("[%s] ai(%d,%d) release frame\n", __FUNCTION__, obj->ai_dev, item->chn);
}