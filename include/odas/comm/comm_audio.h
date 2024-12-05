#ifndef COMM_AUDIO_H
#define COMM_AUDIO_H

#include <pthread.h>

#include "ot_buffer.h"
#include "ot_common.h"
#include "ot_common_adec.h"
#include "ot_common_aenc.h"
#include "ot_defines.h"
#include "ot_math.h"
#include "ot_mipi_rx.h"
#include "ot_mipi_tx.h"
#include "securec.h"
#include "hi_mpi_audio.h"
#include "hi_mpi_sys_bind.h"

#ifdef __cplusplus
extern "C" {
#endif /* end of #ifdef __cplusplus */

/* macro define */
#define FILE_NAME_LEN 128
#define FILE_PATH_LEN 128

#define COLOR_RGB_RED 0xFF0000
#define COLOR_RGB_GREEN 0x00FF00
#define COLOR_RGB_BLUE 0x0000FF
#define COLOR_RGB_BLACK 0x000000
#define COLOR_RGB_YELLOW 0xFFFF00
#define COLOR_RGB_CYN 0x00ffff
#define COLOR_RGB_WHITE 0xffffff

#define SAMPLE_AUDIO_EXTERN_AI_DEV 0
#define SAMPLE_AUDIO_EXTERN_AO_DEV 0
#define SAMPLE_AUDIO_INNER_AI_DEV 0
#define SAMPLE_AUDIO_INNER_AO_DEV 0

#define SAMPLE_AUDIO_POINT_NUM_PER_FRAME 480
#define SAMPLE_AUDIO_AI_USER_FRAME_DEPTH 5
#define SAMPLE_AUDIO_AI_CHANNELS 8

#define WDR_MAX_PIPE_NUM 4

#define CHN_NUM_PRE_DEV 4
#define SECOND_CHN_OFFSET_2MUX 2

#define NVP6158_FILE "/dev/nc_vdec"
#define TP2856_FILE "/dev/tp2802dev"
#define TP2828_FILE "/dev/tp2823dev"

#define ACODEC_FILE "/dev/acodec"

#define ES8388_FILE "/dev/es8388"
#define ES8388_CHIP_ID 0

#define minor_chn(vi_chn) ((vi_chn) + 1)

#define sample_pause()                                                  \
  do {                                                                  \
    printf("---------------press enter key to exit!---------------\n"); \
    getchar();                                                          \
  } while (0)

#define sample_print(fmt...)                     \
  do {                                           \
    printf("[%s]-%d: ", __FUNCTION__, __LINE__); \
    printf(fmt);                                 \
  } while (0)

#define check_null_ptr_return(ptr)                                       \
  do {                                                                   \
    if ((ptr) == TD_NULL) {                                              \
      printf("func:%s,line:%d, NULL pointer\n", __FUNCTION__, __LINE__); \
      return TD_FAILURE;                                                 \
    }                                                                    \
  } while (0)

#define check_chn_return(express, chn, name)                                  \
  do {                                                                        \
    td_s32 ret_ = (express);                                                  \
    if (ret_ != TD_SUCCESS) {                                                 \
      printf(                                                                 \
          "\033[0;31m%s chn %d failed at %s: LINE: %d with %#x!\033[0;39m\n", \
          (name), (chn), __FUNCTION__, __LINE__, ret_);                       \
      fflush(stdout);                                                         \
      return ret_;                                                            \
    }                                                                         \
  } while (0)

#define check_return(express, name)                                       \
  do {                                                                    \
    td_s32 ret_ = (express);                                              \
    if (ret_ != TD_SUCCESS) {                                             \
      printf("\033[0;31m%s failed at %s: LINE: %d with %#x!\033[0;39m\n", \
             (name), __FUNCTION__, __LINE__, ret_);                       \
      return ret_;                                                        \
    }                                                                     \
  } while (0)

#define sample_check_eok_return(ret, err_code)                     \
  do {                                                             \
    if ((ret) != EOK) {                                            \
      printf("%s:%d:strncpy_s failed.\n", __FUNCTION__, __LINE__); \
      return (err_code);                                           \
    }                                                              \
  } while (0)

#define rgn_check_handle_num_return(handle_num)                             \
  do {                                                                      \
    if (((handle_num) < SAMPLE_RGN_HANDLE_NUM_MIN) ||                       \
        ((handle_num) > SAMPLE_RGN_HANDLE_NUM_MAX)) {                       \
      sample_print("handle_num(%d) should be in [%d, %d].\n", (handle_num), \
                   SAMPLE_RGN_HANDLE_NUM_MIN, SAMPLE_RGN_HANDLE_NUM_MAX);   \
      return TD_FAILURE;                                                    \
    }                                                                       \
  } while (0)

#define check_digit(x) ((x) >= '0' && (x) <= '9')

/* structure define */
typedef enum {
  AUDIO_VQE_TYPE_NONE = 0,
  AUDIO_VQE_TYPE_RECORD,
  AUDIO_VQE_TYPE_TALK,
  AUDIO_VQE_TYPE_TALKV2,
  AUDIO_VQE_TYPE_MAX,
} audio_vqe_type;

typedef struct {
  ot_audio_sample_rate out_sample_rate;
  td_bool resample_en;
  td_void *ai_vqe_attr;
  audio_vqe_type ai_vqe_type;
} comm_ai_vqe_param;

td_s32 comm_audio_init(td_void);
td_void comm_audio_exit(td_void);
td_s32 comm_audio_create_thread_ai_ao(ot_audio_dev ai_dev,
                                             ot_ai_chn ai_chn,
                                             ot_audio_dev ao_dev,
                                             ot_ao_chn ao_chn);
td_s32 comm_audio_create_thread_ai_aenc(ot_audio_dev ai_dev,
                                               ot_ai_chn ai_chn,
                                               ot_aenc_chn ae_chn);
td_s32 comm_audio_create_thread_aenc_adec(ot_aenc_chn ae_chn,
                                                 ot_adec_chn ad_chn,
                                                 FILE *aenc_fd);
td_s32 comm_audio_create_thread_file_adec(ot_adec_chn ad_chn,
                                                 FILE *adec_fd);
td_s32 comm_audio_create_thread_ao_vol_ctrl(ot_audio_dev ao_dev);
td_s32 comm_audio_destroy_thread_ai(ot_audio_dev ai_dev,
                                           ot_ai_chn ai_chn);
td_s32 comm_audio_destroy_thread_aenc_adec(ot_aenc_chn ae_chn);
td_s32 comm_audio_destroy_thread_file_adec(ot_adec_chn ad_chn);
td_s32 comm_audio_destroy_thread_ao_vol_ctrl(ot_audio_dev ao_dev);
td_s32 comm_audio_destroy_all_thread(td_void);
td_s32 comm_audio_ao_bind_adec(ot_audio_dev ao_dev, ot_ao_chn ao_chn,
                                      ot_adec_chn ad_chn);
td_s32 comm_audio_ao_unbind_adec(ot_audio_dev ao_dev, ot_ao_chn ao_chn,
                                        ot_adec_chn ad_chn);
td_s32 comm_audio_ao_bind_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn,
                                    ot_audio_dev ao_dev, ot_ao_chn ao_chn);
td_s32 comm_audio_ao_unbind_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn,
                                      ot_audio_dev ao_dev, ot_ao_chn ao_chn);
td_s32 comm_audio_aenc_bind_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn,
                                      ot_aenc_chn ae_chn);
td_s32 comm_audio_aenc_unbind_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn,
                                        ot_aenc_chn ae_chn);
td_s32 comm_audio_start_ai(ot_audio_dev ai_dev_id, td_u32 ai_chn_cnt,
                                  ot_aio_attr *aio_attr,
                                  const comm_ai_vqe_param *ai_vqe_param,
                                  ot_audio_dev ao_dev_id);
td_s32 comm_audio_stop_ai(ot_audio_dev ai_dev_id, td_u32 ai_chn_cnt,
                                 td_bool resample_en, td_bool vqe_en);
td_s32 comm_audio_start_ao(ot_audio_dev ao_dev_id, td_u32 ao_chn_cnt,
                                  ot_aio_attr *aio_attr,
                                  ot_audio_sample_rate in_sample_rate,
                                  td_bool resample_en);
td_s32 comm_audio_stop_ao(ot_audio_dev ao_dev_id, td_u32 ao_chn_cnt,
                                 td_bool resample_en);
td_s32 comm_audio_start_aenc(td_u32 aenc_chn_cnt,
                                    const ot_aio_attr *aio_attr,
                                    ot_payload_type type);
td_s32 comm_audio_stop_aenc(td_u32 aenc_chn_cnt);
td_s32 comm_audio_start_adec(td_u32 adec_chn_cnt,
                                    const ot_aio_attr *aio_attr,
                                    ot_payload_type type);
td_s32 comm_audio_stop_adec(ot_adec_chn ad_chn);
td_s32 comm_audio_cfg_acodec(const ot_aio_attr *aio_attr);

#ifdef __cplusplus
}
#endif /* end of #ifdef __cplusplus */

#endif /* end of #ifndef COMM_AUDIOON_H */
