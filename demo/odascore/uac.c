#include <alsa/asoundlib.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "comm_audio.h"
#include "odascore.h"

#define DMIC_GAIN 90 /* 90: 24dB */
#define PCM_WAIT_TIME_MS 1000
#define ALSA_DEVICE_NAME "default"

volatile sig_atomic_t g_need_quit_flag = 0;
sig_atomic_t uac_get_quit_flag(void) { return g_need_quit_flag; }

typedef struct {
  // channel number
  td_u32 chn;
  // channel FD
  td_s32 ai_fd;
  // read frame
  ot_audio_frame frame;
  // read FD
  fd_set read_fds;
} uac_item;

typedef struct {
  // flag for init
  int init;
  // channel number
  int chn;
  // point num per frame (80/160/240/320/480/1024/2048)
  unsigned int per_frame;
  // frame size
  unsigned int frame_size;
  // device for AI
  ot_audio_dev ai_dev;
  // attributes for AI
  ot_aio_attr ai_attr;
  // sample rate
  ot_audio_sample_rate sample_rate;
  // bit width
  ot_audio_bit_width bit_width;

  // for read each channel data
  uac_item items[SAMPLE_AUDIO_AI_CHANNELS];

  // uac stuff
  snd_pcm_t* uac_hdl;
  snd_pcm_hw_params_t* uac_hw_params;
  snd_pcm_sw_params_t* uac_sw_params;
  snd_pcm_uframes_t uac_period_size;
  unsigned int uac_channels;

  // worker thread
  pthread_t worker;
  atomic_bool running;

  // odascore
  odascore_t* odascore;

} uac_spec;

static FILE* test_input_file = NULL;

static void test_save_to_file(void* buffer, size_t len) {
  if (test_input_file == NULL) {
    test_input_file = fopen("/home/meonardo/bin/sss.pcm", "w+");
    if (test_input_file == NULL) {
      printf("Cannot open file input.pcm\n");
      exit(EXIT_FAILURE);
    }
  }

  fwrite(buffer, 1, len, test_input_file);
}

static int uac_alsa_playback_set_sw_param(uac_spec* obj) {
  int err;
  unsigned int val;

  /* get sw_params */
  err = snd_pcm_sw_params_current(obj->uac_hdl, obj->uac_sw_params);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_sw_params_current error: %s\n", __FUNCTION__,
           __LINE__, snd_strerror(err));
    return err;
  }

  /* start_threshold */
  val = obj->per_frame + 1; /* start after at least 2 frame */
  err = snd_pcm_sw_params_set_start_threshold(obj->uac_hdl, obj->uac_sw_params,
                                              val);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_sw_params_set_start_threshold error: %s\n",
           __FUNCTION__, __LINE__, snd_strerror(err));
    return err;
  }

  /* set the sw_params to the driver */
  err = snd_pcm_sw_params(obj->uac_hdl, obj->uac_sw_params);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_sw_params error: %s\n", __FUNCTION__, __LINE__,
           snd_strerror(err));
    return err;
  }

  return 0;
}

static int uac_alsa_playback_set_hw_param(uac_spec* obj) {
  int err;
  int dir = SND_PCM_STREAM_PLAYBACK;
  unsigned int val;

  /* Interleaved mode */
  err = snd_pcm_hw_params_set_access(obj->uac_hdl, obj->uac_hw_params,
                                     SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_set_access error: %s\n", __FUNCTION__,
           __LINE__, snd_strerror(err));
    return err;
  }

  /* Signed 16-bit little-endian format */
  err = snd_pcm_hw_params_set_format(obj->uac_hdl, obj->uac_hw_params,
                                     SND_PCM_FORMAT_S16_LE);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_set_format error: %s\n", __FUNCTION__,
           __LINE__, snd_strerror(err));
    return err;
  }

  /* channels */
  err = snd_pcm_hw_params_set_channels(obj->uac_hdl, obj->uac_hw_params, 2);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_set_channels error: %s\n", __FUNCTION__,
           __LINE__, snd_strerror);
    return err;
  }

  /* period (default: 1024) */
  val = obj->per_frame;
  err = snd_pcm_hw_params_set_period_size(obj->uac_hdl, obj->uac_hw_params, val,
                                          dir);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_set_period_size error: %s\n",
           __FUNCTION__, __LINE__, snd_strerror(err));
    return err;
  }

  /* buf size */
  val = obj->per_frame * 4;
  err =
      snd_pcm_hw_params_set_buffer_size(obj->uac_hdl, obj->uac_hw_params, val);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_set_buffer_size error: %s\n",
           __FUNCTION__, __LINE__, snd_strerror(err));
    return err;
  }

  /* sampling rate */
  val = obj->sample_rate;
  err = snd_pcm_hw_params_set_rate_near(obj->uac_hdl, obj->uac_hw_params, &val,
                                        &dir);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_set_rate_near error: %s\n", __FUNCTION__,
           __LINE__, snd_strerror(err));
    return err;
  }

  /* Write the parameters to the driver */
  err = snd_pcm_hw_params(obj->uac_hdl, obj->uac_hw_params);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params error: %s\n", __FUNCTION__, __LINE__,
           snd_strerror(err));
    return err;
  }

  return 0;
}

static int uac_alsa_playback_get_period_and_channel(
    uac_spec* obj, snd_pcm_uframes_t* period_size, unsigned int* channels) {
  int err, dir;

  err =
      snd_pcm_hw_params_get_period_size(obj->uac_hw_params, period_size, &dir);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_get_period_size error: %s\n",
           __FUNCTION__, __LINE__, snd_strerror(err));
    return err;
  } else if ((err > 0) && (*period_size == 0)) {
    *period_size = err;
  }

  err = snd_pcm_hw_params_get_channels(obj->uac_hw_params, channels);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_get_channels error: %s\n", __FUNCTION__,
           __LINE__, snd_strerror(err));
    return err;
  }

  return 0;
}

static int uac_alsa_playback_init(uac_spec* obj) {
  /* alsa */
  int err;

  /* Open PCM device for playback. */
  err =
      snd_pcm_open(&obj->uac_hdl, ALSA_DEVICE_NAME, SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    printf("[%s:%d] audio open error: %s\n", __FUNCTION__, __LINE__,
           snd_strerror(err));
    return err;
  }

  err = snd_pcm_hw_params_malloc(&obj->uac_hw_params);
  if (err < 0) {
    printf("[%s:%d] snd_pcm hardware error: %s", __FUNCTION__, __LINE__,
           snd_strerror(err));
    return err;
  }

  err = snd_pcm_sw_params_malloc(&obj->uac_sw_params);
  if (err < 0) {
    printf("[%s:%d] snd_pcm software error: %s", __FUNCTION__, __LINE__,
           snd_strerror(err));
    return err;
  }

  /* Fill it in with default values. */
  err = snd_pcm_hw_params_any(obj->uac_hdl, obj->uac_hw_params);
  if (err < 0) {
    printf("[%s:%d] snd_pcm hardware error: %s", __FUNCTION__, __LINE__,
           snd_strerror(err));
    return err;
  }

  err = uac_alsa_playback_set_hw_param(obj);
  if (err != 0) {
    printf("[%s:%d] fail to set hw_param\n", __FUNCTION__, __LINE__);
    return err;
  }

  err = uac_alsa_playback_set_sw_param(obj);
  if (err != 0) {
    printf("[%s:%d] fail to set sw_param\n", __FUNCTION__, __LINE__);
    return err;
  }

  err = uac_alsa_playback_get_period_and_channel(obj, &obj->uac_period_size,
                                                 &obj->uac_channels);
  if (err != 0) {
    printf("[%s:%d] fail to get period and channel\n", __FUNCTION__, __LINE__);
    return err;
  }

  return 0;
}

static void uac_alsa_playback_deinit(uac_spec* obj) {
  if (obj->uac_sw_params != NULL) {
    snd_pcm_sw_params_free(obj->uac_sw_params);
    obj->uac_sw_params = NULL;
  }

  if (obj->uac_hw_params != NULL) {
    snd_pcm_hw_params_free(obj->uac_hw_params);
    obj->uac_hw_params = NULL;
  }

  if (obj->uac_hdl != NULL) {
    snd_pcm_close(obj->uac_hdl);
    obj->uac_hdl = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////////

static void on_odasbuf(const char* buf, size_t len, void* user_data) {
  printf("on_odasbuf: len=%zu\n", len);
  
  test_save_to_file(buf, len);

//   uac_spec* obj = (uac_spec*)user_data;

//   // send to UAC
//   int err;
//   int send_finish_status = 0;

//   while (send_finish_status == 0) {
//     err = snd_pcm_wait(obj->uac_hdl, PCM_WAIT_TIME_MS);
//     if (err == 0) {
//       continue;
//     }

//     err = snd_pcm_writei(obj->uac_hdl, buf, obj->uac_period_size);
//     if (err == -EPIPE) {
//       /* EPIPE means underrun */
//       printf("[%s:%d] snd_pcm_writei: underrun occurred, err=%d \n",
//              __FUNCTION__, __LINE__, err);
//       snd_pcm_prepare(obj->uac_hdl);
//     } else if (err < 0) {
//       printf("[%s:%d] snd_pcm_writei: error from writei: %s\n", __FUNCTION__,
//              __LINE__, snd_strerror(err));
//       break;
//     } else if (err != (int)obj->uac_period_size) {
//       printf("[%s:%d] snd_pcm_writei: wrote %d frames\n", __FUNCTION__,
//              __LINE__, err);
//       break;
//     } else {
//       send_finish_status = 1;
//     }
//   }
}

/// ai configurations
static td_void init_params(ot_aio_attr* ai_attr, ot_audio_dev* ai_dev,
                           uac_spec* obj) {
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

static void* uac_worker(void* arg) {
  int ret;
  uac_spec* obj = (uac_spec*)arg;

  while (obj->running) {
    char* chn_bufs[8] = {};
    // read frame from the ai chn
    for (int i = 0; i < obj->chn; i++) {
      uac_item* item = &obj->items[i];
      ret = app_audio_select(item->ai_fd, &item->read_fds);
      if (ret != TD_SUCCESS) {
        printf("%s: app_audio_select failed with %d!\n", __FUNCTION__, ret);
        break;
      }

      if (FD_ISSET(item->ai_fd, &(item->read_fds))) {
        /* get frame from ai chn */
        ret = hi_mpi_ai_get_frame(obj->ai_dev, item->chn, &item->frame, NULL,
                                  TD_FALSE);
        if (ret != TD_SUCCESS) {
          printf("[%s] hi_mpi_ai_get_frame failed with %d!\n", __FUNCTION__,
                 ret);
          break;
        }

        chn_bufs[i] = item->frame.virt_addr[0];
      }
    }

    // send the audio buffer to odascore
    if (obj->odascore != NULL) {
      odascore_put_frames(obj->odascore, chn_bufs, obj->chn);
    }

    // release frame
    for (int i = 0; i < obj->chn; i++) {
      uac_item* item = &obj->items[i];
      ret = hi_mpi_ai_release_frame(obj->ai_dev, item->chn, &item->frame, NULL);
      if (ret != TD_SUCCESS) {
        printf("[%s] hi_mpi_ai_release_frame failed with %d!\n", __FUNCTION__,
               ret);
        break;
      }
    }
  }

  return NULL;
}

static int uac_init(uac_spec* obj) {
  if (obj->init == 1) {
    printf("[%s] uac_spec already init!\n", __FUNCTION__);
    return 0;
  }

  int ret;

  // init audio module
  ret = comm_audio_init();
  if (ret != 0) {
    printf("[%s] audio init failed with %d!\n", __FUNCTION__, ret);
    return ret;
  }

  comm_ai_vqe_param ai_vqe_param = {};

  // init audio input params
  init_params(&(obj->ai_attr), &obj->ai_dev, obj);

  // start ai
  ret = comm_audio_start_ai(obj->ai_dev, obj->chn, &obj->ai_attr, &ai_vqe_param,
                            -1);
  if (ret != TD_SUCCESS) {
    printf("[%s] comm_audio_start_ai failed with %d!\n", __FUNCTION__, ret);
    return ret;
  }

  // set dmic gain
  ret = hi_mpi_ai_set_dmic_gain(obj->ai_dev, DMIC_GAIN);
  if (ret != TD_SUCCESS) {
    printf("[%s] hi_mpi_ai_set_dmic_gain failed with %d!\n", __FUNCTION__, ret);
    comm_audio_stop_ai(obj->ai_dev, obj->chn, TD_FALSE, TD_TRUE);
    return ret;
  }

  // init items
  for (int i = 0; i < obj->chn; i++) {
    obj->items[i].chn = i;
    obj->items[i].ai_fd = -1;

    ot_ai_chn_param ai_chn_para;
    ret = hi_mpi_ai_get_chn_param(obj->ai_dev, i, &ai_chn_para);
    if (ret != TD_SUCCESS) {
      printf("%s: get ai chn param failed\n", __FUNCTION__);
      return TD_FAILURE;
    }
    ai_chn_para.usr_frame_depth = SAMPLE_AUDIO_AI_USER_FRAME_DEPTH;
    ret = hi_mpi_ai_set_chn_param(obj->ai_dev, i, &ai_chn_para);
    if (ret != TD_SUCCESS) {
      printf("%s: set ai chn param failed, ret=0x%x\n", __FUNCTION__, ret);
      return TD_FAILURE;
    }

    td_s32 ai_fd = hi_mpi_ai_get_fd(obj->ai_dev, i);
    if (ai_fd < 0) {
      printf("%s: get ai fd failed\n", __FUNCTION__);
      return TD_FAILURE;
    }

    obj->items[i].ai_fd = ai_fd;

    printf("[%s] ai(%d,%d) fd = %d\n", __FUNCTION__, obj->ai_dev, i, ai_fd);
  }

  // init alsa playback
  ret = uac_alsa_playback_init(obj);

  // init the odascore
  obj->odascore =
      odascore_construct("/home/meonardo/bin/8.cfg", 0, on_odasbuf, obj);

  // launch a worker thread to capture audio data
  obj->running = 1;
  ret = pthread_create(&obj->worker, NULL, uac_worker, obj);
  if (ret != 0) {
    printf("[%s] pthread_create failed with %d!\n", __FUNCTION__, ret);
    return ret;
  }

  // mark as init
  obj->init = 1;

  printf("[%s] uac_spec init success!\n\n\n", __FUNCTION__);

  return 0;
}

static void uac_destory(uac_spec* obj) {
  if (obj->init == 0) {
    printf("[%s] uac_spec already destory!\n", __FUNCTION__);
    return;
  }

  obj->init = 0;

  // stop worker thread
  obj->running = 0;
  pthread_join(obj->worker, NULL);

  // alsa playback deinit
  uac_alsa_playback_deinit(obj);

  // odascore destory
  odascore_destroy(obj->odascore);

  // stop ai
  comm_audio_stop_ai(obj->ai_dev, obj->chn, TD_FALSE, TD_TRUE);

  // destory module
  comm_audio_exit();

  printf("[%s] uac_spec destory success!\n\n\n", __FUNCTION__);
}

static td_s32 uac_read_frame(uac_spec* obj, uac_item* item) {
  td_s32 ret;

  ret = app_audio_select(item->ai_fd, &item->read_fds);
  if (ret != TD_SUCCESS) {
    printf("%s: app_audio_select failed with %d!\n", __FUNCTION__, ret);
    return TD_FAILURE;
  }

  if (FD_ISSET(item->ai_fd, &(item->read_fds))) {
    /* get frame from ai chn */
    ret = hi_mpi_ai_get_frame(obj->ai_dev, item->chn, &item->frame, NULL,
                              TD_FALSE);
    if (ret != TD_SUCCESS) {
      printf("[%s] hi_mpi_ai_get_frame failed with %d!\n", __FUNCTION__, ret);
      return TD_FAILURE;
    }
  }

  return TD_SUCCESS;
}

static void uac_release_frame(uac_spec* obj, uac_item* item) {
  td_s32 ret =
      hi_mpi_ai_release_frame(obj->ai_dev, item->chn, &item->frame, NULL);
  if (ret != TD_SUCCESS) {
    printf("[%s] hi_mpi_ai_release_frame(%d, %d), failed with %#x!\n",
           __FUNCTION__, obj->ai_dev, item->chn, ret);
    return;
  }
}

static void uac_handle_signal(int signo) {
  if (signo == SIGINT || signo == SIGTERM) {
    g_need_quit_flag = 1;
  }
}

int main() {
  struct sigaction act = {0};
  act.sa_handler = uac_handle_signal;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);

  uac_spec obj = {0};
  obj.chn = 8;
  obj.per_frame = 480;
  obj.sample_rate = OT_AUDIO_SAMPLE_RATE_48000;
  obj.frame_size = 2;
  obj.bit_width = OT_AUDIO_BIT_WIDTH_16;

  if (uac_init(&obj) != 0) {
    printf("%s,%d Cannot init uac source.\n", __FUNCTION__, __LINE__);
    return -1;
  }

  while (HI_TRUE) {
    if (uac_get_quit_flag() != 0) {
      break;
    }
    usleep(10 * 1000); /* 10 * 1000us sleep */
  }

  uac_destory(&obj);

  return 0;
}