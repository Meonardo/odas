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

static FILE* test_input_file = NULL;

static void src_hops_save_to_file1(void* buffer, size_t len) {
  if (test_input_file == NULL) {
    test_input_file = fopen("/home/meonardo/bin/input1.pcm", "w+");
    if (test_input_file == NULL) {
      printf("Cannot open file input.pcm\n");
      exit(EXIT_FAILURE);
    }
  }

  fwrite(buffer, 1, len, test_input_file);
}

#ifdef UAC_SRC_SINK
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

static void* src_pcm_worker(void* arg) {
  int ret;
  src_pcm_spec* obj = (src_pcm_spec*)arg;

  // create a ring buffer
  if (obj->rb == NULL) {
    obj->rb = (ring_buffer_t*)malloc(sizeof(ring_buffer_t));
    size_t buffer_size = obj->per_frame * obj->chn * 2;
    obj->buffer = (uint8_t*)malloc(buffer_size);
    ret = ring_buffer_init(obj->rb, buffer_size * 4);
    if (ret != 0) {
      printf("[%s] ring_buffer_init failed with %d!\n", __FUNCTION__, ret);
      return NULL;
    }
  }

  while (obj->running) {

    // read frame from the ai chn
    for (int i = 0; i < obj->chn; i++) {
      src_pcm_item* item = &obj->items[i];
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
      }
    }

    // interleave the frame data
    int offset = 0;
    int size_per_sample = sizeof(int16_t); // 2 bytes
    int samples = obj->per_frame;          // 1024 samples
    for (int i = 0; i < samples; i++) {
      for (int j = 0; j < obj->chn; j++) {
        src_pcm_item* item = &obj->items[j];
        td_u8* data = item->frame.virt_addr[0];
        offset = i * obj->chn * size_per_sample + j * size_per_sample;
        memcpy(obj->buffer + offset, data + i * size_per_sample,
               size_per_sample);
      }
    }

    // src_hops_save_to_file1(obj->buffer, samples * obj->chn * size_per_sample);

    // write to ring buffer
    ring_buffer_write(obj->rb, obj->buffer,
                      samples * obj->chn * size_per_sample, 0);

    // release frame
    for (int i = 0; i < obj->chn; i++) {
      src_pcm_item* item = &obj->items[i];
      ret = hi_mpi_ai_release_frame(obj->ai_dev, item->chn, &item->frame, NULL);
      if (ret != TD_SUCCESS) {
        printf("[%s] hi_mpi_ai_release_frame failed with %d!\n", __FUNCTION__, ret);
        break;
      }
    }
  }

  // free resources
  if (obj->rb != NULL) {
    ring_buffer_release(obj->rb);
    free(obj->rb);
    free(obj->buffer);
    obj->buffer = NULL;
  }

  return NULL;
}

#endif

int src_pcm_init(src_pcm_spec* obj) {
  if (obj->init == 1) {
    printf("[%s] src_pcm_spec already init!\n", __FUNCTION__);
    return 0;
  }

  int ret;

#ifdef UAC_SRC_SINK
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
  ret = comm_audio_start_ai(obj->ai_dev, obj->chn, &obj->ai_attr,
                            &ai_vqe_param, -1);
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

  // launch a worker thread to capture audio data
  obj->running = 1;
  ret = pthread_create(&obj->worker, NULL, src_pcm_worker, obj);
  if (ret != 0) {
    printf("[%s] pthread_create failed with %d!\n", __FUNCTION__, ret);
    return ret;
  }
#else
  // create a ring buffer
  obj->rb = (ring_buffer_t*)malloc(sizeof(ring_buffer_t));
  size_t buffer_size = obj->per_frame * obj->chn * obj->frame_size;
  obj->buffer = (char*)malloc(buffer_size);
  ret = ring_buffer_init(obj->rb, buffer_size * RB_COUNT);
  if (ret != 0) {
    printf("[%s] ring_buffer_init failed with %d!\n", __FUNCTION__, ret);
    return ret;
  }
#endif

  // mark as init
  obj->init = 1;

  printf("[%s] src_pcm_spec init success!\n\n\n", __FUNCTION__);

  return 0;
}

void src_pcm_destory(src_pcm_spec* obj) {
  if (obj->init == 1) {
    printf("[%s] src_pcm_spec already destory!\n", __FUNCTION__);
    return;
  }

  obj->init = 0;

#ifdef UAC_SRC_SINK
  // stop worker thread
  obj->running = 0;
  pthread_join(obj->worker, NULL);

  // stop ai
  comm_audio_stop_ai(obj->ai_dev, obj->chn, TD_FALSE, TD_TRUE);

  // destory module
  comm_audio_exit();
#else
  // free resources
  if (obj->rb != NULL) {
    ring_buffer_release(obj->rb);
    free(obj->rb);
    free(obj->buffer);
    obj->buffer = NULL;
  }  
#endif  

  printf("[%s] src_pcm_spec destory success!\n\n\n", __FUNCTION__);
}

#ifdef UAC_SRC_SINK
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
        hi_mpi_ai_get_frame(obj->ai_dev, item->chn, &item->frame, NULL, TD_FALSE);
    if (ret != TD_SUCCESS) {
      printf("[%s] hi_mpi_ai_get_frame failed with %d!\n", __FUNCTION__, ret);
      return TD_FAILURE;
    }

    // printf("[%s] ai(%d,%d) get frame, len = %d\n", __FUNCTION__, obj->ai_dev, item->chn,
    //        item->frame.len);
  }

  return TD_SUCCESS;
}

void src_pcm_release_frame(src_pcm_spec* obj, src_pcm_item* item) {
  td_s32 ret =
      hi_mpi_ai_release_frame(obj->ai_dev, item->chn, &item->frame, NULL);
  if (ret != TD_SUCCESS) {
    printf("[%s] hi_mpi_ai_release_frame(%d, %d), failed with %#x!\n",
           __FUNCTION__, obj->ai_dev, item->chn, ret);
    return;
  }

  // printf("[%s] ai(%d,%d) release frame\n", __FUNCTION__, obj->ai_dev, item->chn);
}
#endif

int src_pcm_read_buffer(src_pcm_spec* obj, char* buffer, size_t size) {
  return ring_buffer_read(obj->rb, buffer, size);
}

int src_pcm_write_buffer(src_pcm_spec* obj, char** chn_bufs, int chn) {
  // interleave the frame data
  int offset = 0;
  int size_per_sample = obj->frame_size;  // bytes per sample
  int samples = obj->per_frame;           // samples
  for (int i = 0; i < samples; i++) {
    for (int j = 0; j < obj->chn; j++) {
      char* data = chn_bufs[j];
      offset = i * obj->chn * size_per_sample + j * size_per_sample;
      memcpy(obj->buffer + offset, data + i * size_per_sample, size_per_sample);
    }
  }

  // src_hops_save_to_file1(obj->buffer, samples * obj->chn * size_per_sample);

  // write to ring buffer
  return ring_buffer_write(obj->rb, obj->buffer,
                           samples * obj->chn * size_per_sample, 0);
}