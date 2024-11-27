#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "sample_comm.h"

static td_bool g_aio_resample = TD_FALSE;

static td_bool g_sample_audio_exit = TD_FALSE;

#define sample_dbg(ret)                                                  \
  do {                                                                   \
    printf("ret = %#x, fuc:%s, line:%d\n", ret, __FUNCTION__, __LINE__); \
  } while (0)

#define sample_res_check_null_ptr(ptr)                                   \
  do {                                                                   \
    if ((td_u8 *)(ptr) == TD_NULL) {                                     \
      printf("ptr is TD_NULL,fuc:%s,line:%d\n", __FUNCTION__, __LINE__); \
      return TD_FAILURE;                                                 \
    }                                                                    \
  } while (0)

#define SAMPLE_DMIC_GAIN 90 /* 90: 24dB */

typedef struct {
  td_bool start;
  td_s32 ai_dev;
  td_s32 ai_chn;
  ot_audio_sample_rate ai_sample;
  td_u32 per_frame;
  FILE *fd;
  pthread_t ai_pid;
} ot_sample_ai_to_file;

static ot_sample_ai_to_file
    g_sample_ai_ext_res[OT_AI_DEV_MAX_NUM * OT_AI_MAX_CHN_NUM];

static td_s32 sample_comm_audio_select(td_s32 ai_fd, fd_set *read_fds) {
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
    printf("%s: get ai frame select time out\n", __FUNCTION__);
    return TD_FAILURE;
  }
  return TD_SUCCESS;
}

/* function : get frame from Ai, write it to file */
void *sample_comm_audio_ai_to_file_proc(void *parg) {
  td_s32 ret;
  td_s32 ai_fd = -1;
  ot_audio_frame frame = {0};
  ot_aec_frame aec_frm = {0};
  ot_sample_ai_to_file *ai_ctrl = (ot_sample_ai_to_file *)parg;
  fd_set read_fds;
  td_u16 cache_count = 0;
  ot_ai_chn_param ai_chn_para;
  td_s16 cache[16]; /* 16:Max 64 / 8 * 2. */

  // init `ot_sample_ai_to_file`
  ret = ss_mpi_ai_get_chn_param(ai_ctrl->ai_dev, ai_ctrl->ai_chn, &ai_chn_para);
  if (ret != TD_SUCCESS) {
    printf("%s: get ai chn param failed\n", __FUNCTION__);
    return TD_NULL;
  }
  ai_chn_para.usr_frame_depth = SAMPLE_AUDIO_AI_USER_FRAME_DEPTH;
  ret = ss_mpi_ai_set_chn_param(ai_ctrl->ai_dev, ai_ctrl->ai_chn, &ai_chn_para);
  if (ret != TD_SUCCESS) {
    printf("%s: set ai chn param failed, ret=0x%x\n", __FUNCTION__, ret);
    return TD_NULL;
  }

  ai_fd = ss_mpi_ai_get_fd(ai_ctrl->ai_dev, ai_ctrl->ai_chn);
  if (ai_fd < 0) {
    printf("%s: get ai fd failed\n", __FUNCTION__);
    return TD_NULL;
  }

  while (ai_ctrl->start) {
    ret = sample_comm_audio_select(ai_fd, &read_fds);
    if (ret != TD_SUCCESS) {
      break;
    }

    if (FD_ISSET(ai_fd, &read_fds)) {
      /* get frame from ai chn */
      (td_void)
          memset_s(&aec_frm, sizeof(ot_aec_frame), 0, sizeof(ot_aec_frame));

      ret = ss_mpi_ai_get_frame(ai_ctrl->ai_dev, ai_ctrl->ai_chn, &frame,
                                &aec_frm, TD_FALSE);
      if (ret != TD_SUCCESS) {
        continue;
      }

      // write to file
      fwrite(frame.virt_addr[0], 1, frame.len, ai_ctrl->fd);

      // flush file
      (void)fflush(ai_ctrl->fd);

      ret = ss_mpi_ai_release_frame(ai_ctrl->ai_dev, ai_ctrl->ai_chn, &frame,
                                    &aec_frm);
      if (ret != TD_SUCCESS) {
        printf("%s: ss_mpi_ai_release_frame(%d, %d), failed with %#x!\n",
               __FUNCTION__, ai_ctrl->ai_dev, ai_ctrl->ai_chn, ret);
        break;
      }
    }
  }

  ai_ctrl->start = TD_FALSE;

  return TD_NULL;
}

// create worker
td_s32 sample_comm_audio_create_thread(ot_aio_attr *aio_attr,
                                                  ot_audio_dev ai_dev,
                                                  ot_ai_chn ai_chn,
                                                  FILE *res_fd) {
  ot_sample_ai_to_file *ai_to_ext_res = TD_NULL;

  if ((ai_dev >= OT_AI_DEV_MAX_NUM) || (ai_dev < 0) ||
      (ai_chn >= OT_AI_MAX_CHN_NUM) || (ai_chn < 0)) {
    printf("%s: ai_dev = %d, ai_chn = %d error.\n", __FUNCTION__, ai_dev,
           ai_chn);
    return TD_FAILURE;
  }

  ai_to_ext_res = &g_sample_ai_ext_res[ai_dev * OT_AI_MAX_CHN_NUM + ai_chn];
  ai_to_ext_res->ai_dev = ai_dev;
  ai_to_ext_res->ai_chn = ai_chn;
  ai_to_ext_res->ai_sample = aio_attr->sample_rate;
  ai_to_ext_res->per_frame = aio_attr->point_num_per_frame;
  ai_to_ext_res->fd = res_fd;
  ai_to_ext_res->start = TD_TRUE;
  pthread_create(&ai_to_ext_res->ai_pid, 0, sample_comm_audio_ai_to_file_proc,
                 ai_to_ext_res);

  return TD_SUCCESS;
}

// destroy worker
td_s32 sample_comm_audio_destroy_thread(ot_audio_dev ai_dev,
                                                   ot_ai_chn ai_chn) {
  ot_sample_ai_to_file *ai_to_ext_res = TD_NULL;

  if ((ai_dev >= OT_AI_DEV_MAX_NUM) || (ai_dev < 0) ||
      (ai_chn >= OT_AI_MAX_CHN_NUM) || (ai_chn < 0)) {
    printf("%s: ai_dev = %d, ai_chn = %d error.\n", __FUNCTION__, ai_dev,
           ai_chn);
    return TD_FAILURE;
  }

  ai_to_ext_res = &g_sample_ai_ext_res[ai_dev * OT_AI_MAX_CHN_NUM + ai_chn];
  if (ai_to_ext_res->start) {
    ai_to_ext_res->start = TD_FALSE;
    pthread_join(ai_to_ext_res->ai_pid, 0);
  }

  if (ai_to_ext_res->fd != TD_NULL) {
    fclose(ai_to_ext_res->fd);
    ai_to_ext_res->fd = TD_NULL;
  }

  return TD_SUCCESS;
}

static td_void sample_audio_exit_proc(td_void) {
  td_u32 dev_id, chn_id;

  for (dev_id = 0; dev_id < OT_AI_DEV_MAX_NUM; dev_id++) {
    for (chn_id = 0; chn_id < OT_AI_MAX_CHN_NUM; chn_id++) {
      if (sample_comm_audio_destroy_thread(dev_id, chn_id) !=
          TD_SUCCESS) {
        printf(
            "%s: sample_comm_audio_destroy_thread(%d,%d) failed!\n",
            __FUNCTION__, dev_id, chn_id);
      }
    }
  }
  sample_comm_audio_destroy_all_thread();
  sample_comm_audio_exit();
}

static int smaple_audio_getchar(td_void) {
  int c;

  if (g_sample_audio_exit == TD_TRUE) {
    sample_audio_exit_proc();
    printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    exit(-1);
  }

  c = getchar();

  if (g_sample_audio_exit == TD_TRUE) {
    sample_audio_exit_proc();
    printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    exit(-1);
  }

  return c;
}

/* function : Open ExtResample File */
static FILE *sample_audio_open_file(ot_ai_chn ai_chn) {
  FILE *fd = TD_NULL;
  td_char asz_file_name[FILE_NAME_LEN] = {0};
  td_s32 ret;

  /* create file for save stream */
  ret = snprintf_s(asz_file_name, FILE_NAME_LEN, FILE_NAME_LEN - 1,
                   "chn%d.pcm", ai_chn);
  if (ret < 0) {
    printf("[func]:%s [line]:%d [info]:%s\n", __FUNCTION__, __LINE__,
           "get file name failed");
    return TD_NULL;
  }

  if (asz_file_name[0] == '\0') {
    printf("[func]:%s [line]:%d [info]:%s\n", __FUNCTION__, __LINE__,
           "file name is NULL");
    return TD_NULL;
  }

  if (strlen(asz_file_name) > (FILE_NAME_LEN - 1)) {
    printf("[func]:%s [line]:%d [info]:%s\n", __FUNCTION__, __LINE__,
           "file name extra long");
    return TD_NULL;
  }

  fd = fopen(asz_file_name, "w+");
  if (fd == TD_NULL) {
    printf("%s: open file %s failed\n", __FUNCTION__, asz_file_name);
    return TD_NULL;
  }

  printf("open stream file: \"%s\" ok\n", asz_file_name);

  return fd;
}

static td_s32 sample_audio_start_ai_to_file(td_u32 ai_chn_cnt,
                                            ot_audio_dev ai_dev,
                                            ot_aio_attr *aio_attr) {
  td_s32 ret;
  td_u32 i, j;
  ot_ai_chn ai_chn;
  FILE *fd = TD_NULL;

  for (i = 0; i < ai_chn_cnt; i++) {
    ai_chn = i;

    fd = sample_audio_open_file(ai_chn);
    if (fd == TD_NULL) {
      sample_dbg(TD_FAILURE);
      for (j = 0; j < i; j++) {
        sample_comm_audio_destroy_thread(ai_dev, j);
      }
      return TD_FAILURE;
    }

    ret = sample_comm_audio_create_thread(aio_attr, ai_dev, ai_chn,
                                                     fd);
    if (ret != TD_SUCCESS) {
      sample_dbg(ret);
      fclose(fd);
      fd = TD_NULL;
      for (j = 0; j < i; j++) {
        sample_comm_audio_destroy_thread(ai_dev, j);
      }
      return TD_FAILURE;
    }

    printf("ai(%d,%d) write file ok!\n", ai_dev, ai_chn);
  }

  return TD_SUCCESS;
}

static td_void sample_audio_stop_ai_write_to_file(td_u32 ai_chn_cnt,
                                             ot_audio_dev ai_dev) {
  td_s32 ret;
  td_u32 i;
  ot_ai_chn ai_chn;

  for (i = 0; i < ai_chn_cnt; i++) {
    ai_chn = i;
    ret = sample_comm_audio_destroy_thread(ai_dev, ai_chn);
    if (ret != TD_SUCCESS) {
      sample_dbg(ret);
    }
  }
}

static td_void sample_audio_dmic_ai_to_file_init_params(ot_aio_attr *ai_attr,
                                                        ot_audio_dev *ai_dev) {
  ai_attr->sample_rate = OT_AUDIO_SAMPLE_RATE_48000;
  ai_attr->bit_width = OT_AUDIO_BIT_WIDTH_16;
  ai_attr->work_mode = OT_AIO_MODE_I2S_MASTER;
  ai_attr->snd_mode = OT_AUDIO_SOUND_MODE_MONO;
  ai_attr->expand_flag = 0;
  ai_attr->frame_num = SAMPLE_AUDIO_AI_USER_FRAME_DEPTH;
  ai_attr->point_num_per_frame = SAMPLE_AUDIO_POINT_NUM_PER_FRAME;
  ai_attr->chn_cnt = SAMPLE_AUDIO_AI_CHANNELS;

  *ai_dev = SAMPLE_AUDIO_INNER_AI_DEV;
  ai_attr->clk_share = 0;
  ai_attr->i2s_type = OT_AIO_I2STYPE_DMIC;
}

td_s32 sample_audio_dmic_ai_to_file(td_void) {
  td_s32 ret;
  ot_audio_dev ai_dev;
  td_u32 ai_chn_cnt;
  ot_aio_attr ai_attr = {0};
  sample_comm_ai_vqe_param ai_vqe_param = {0};

  // init audio input params
  sample_audio_dmic_ai_to_file_init_params(&ai_attr, &ai_dev);

  // start ai
  ai_chn_cnt = ai_attr.chn_cnt;
  ret = sample_comm_audio_start_ai(ai_dev, ai_chn_cnt, &ai_attr, &ai_vqe_param,
                                   -1);
  if (ret != TD_SUCCESS) {
    sample_dbg(ret);
    return ret;
  }

  // set dmic gain
  ret = ss_mpi_ai_set_dmic_gain(ai_dev, SAMPLE_DMIC_GAIN);
  if (ret != TD_SUCCESS) {
    sample_dbg(ret);
    sample_comm_audio_stop_ai(ai_dev, ai_chn_cnt, g_aio_resample, TD_TRUE);
    return ret;
  }

  // start record & write to file
  ret = sample_audio_start_ai_to_file(ai_chn_cnt, ai_dev, &ai_attr);

  printf("\nplease press twice ENTER to exit this sample\n");
  smaple_audio_getchar();
  smaple_audio_getchar();

  // stop write file
  sample_audio_stop_ai_write_to_file(ai_chn_cnt, ai_dev);

  // stop ai
  sample_comm_audio_stop_ai(ai_dev, ai_chn_cnt, g_aio_resample, TD_TRUE);

  return TD_SUCCESS;
}

/* function : to process abnormal case */
static td_void sample_audio_handle_sig(td_s32 signo) {
  if (g_sample_audio_exit == TD_TRUE) {
    return;
  }

  if ((signo == SIGINT) || (signo == SIGTERM)) {
    g_sample_audio_exit = TD_TRUE;
  }
}

td_void sample_sys_signal(void (*func)(int)) {
  struct sigaction sa = {0};

  sa.sa_handler = func;
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, TD_NULL);
  sigaction(SIGTERM, &sa, TD_NULL);
}

td_s32 main(int argc, char *argv[]) {
  td_s32 ret;

  sample_sys_signal(&sample_audio_handle_sig);

  ret = sample_comm_audio_init();
  if (ret != TD_SUCCESS) {
    printf("%s: audio init failed with %d!\n", __FUNCTION__, ret);
    return TD_FAILURE;
  }

  // ai(dmic)->file
  sample_audio_dmic_ai_to_file();

  sample_comm_audio_exit();

  return ret;
}
