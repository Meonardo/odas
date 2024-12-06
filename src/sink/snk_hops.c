
/**
 * \file     snk_hops.c
 * \author   François Grondin <francois.grondin2@usherbrooke.ca>
 * \version  2.0
 * \date     2018-03-18
 * \copyright
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <sink/snk_hops.h>

#define PCM_WAIT_TIME_MS 1000

static FILE *test_input_file = NULL;

static void snk_hops_save_to_file(msg_hops_obj *obj) {
  if (test_input_file == NULL) {
    test_input_file = fopen("/home/meonardo/bin/output_float.pcm", "w+");
    if (test_input_file == NULL) {
      printf("Cannot open file input.pcm\n");
      exit(EXIT_FAILURE);
    }
  }

  // float
  for (int s = 0; s < obj->hops->hopSize; s++) {
    for (int i = 0; i < obj->hops->nSignals; i++) {
      fwrite(&obj->hops->array[i][s], sizeof(float), 1, test_input_file);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
/// UAC stuff
#ifdef UAC_SRC_SINK

static int uac_alsa_playback_set_sw_param(snk_hops_obj *obj) {
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
  val = obj->hopSize + 1; /* start after at least 2 frame */
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

static int uac_alsa_playback_set_hw_param(snk_hops_obj *obj) {
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
  val = obj->hopSize;
  err = snd_pcm_hw_params_set_period_size(obj->uac_hdl, obj->uac_hw_params, val,
                                          dir);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_set_period_size error: %s\n",
           __FUNCTION__, __LINE__, snd_strerror(err));
    return err;
  }

  /* buf size */
  val = obj->hopSize * 4;
  err =
      snd_pcm_hw_params_set_buffer_size(obj->uac_hdl, obj->uac_hw_params, val);
  if (err < 0) {
    printf("[%s:%d] snd_pcm_hw_params_set_buffer_size error: %s\n",
           __FUNCTION__, __LINE__, snd_strerror(err));
    return err;
  }

  /* sampling rate */
  val = obj->fS;
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
    snk_hops_obj *obj, snd_pcm_uframes_t *period_size, unsigned int *channels) {
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

static int uac_alsa_playback_init(snk_hops_obj *obj) {
  /* alsa */
  int err;

  /* Open PCM device for playback. */
  err = snd_pcm_open(&obj->uac_hdl, obj->interface->deviceName,
                     SND_PCM_STREAM_PLAYBACK, 0);
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

static void uac_alsa_playback_deinit(snk_hops_obj *obj) {
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
#endif

//////////////////////////////////////////////////////////////////////////

snk_hops_obj *snk_hops_construct(const snk_hops_cfg *snk_hops_config,
                                 const msg_hops_cfg *msg_hops_config) {
  snk_hops_obj *obj;

  obj = (snk_hops_obj *)malloc(sizeof(snk_hops_obj));

  obj->timeStamp = 0;

  obj->hopSize = msg_hops_config->hopSize;
  obj->nChannels = msg_hops_config->nChannels;
  obj->fS = snk_hops_config->fS;

  obj->format = format_clone(snk_hops_config->format);
  obj->interface = interface_clone(snk_hops_config->interface);

  memset(obj->bytes, 0x00, 4 * sizeof(char));

  if (!(((obj->interface->type == interface_blackhole) &&
         (obj->format->type == format_undefined)) ||
        ((obj->interface->type == interface_file) &&
         (obj->format->type == format_binary_int08)) ||
        ((obj->interface->type == interface_file) &&
         (obj->format->type == format_binary_int16)) ||
        ((obj->interface->type == interface_file) &&
         (obj->format->type == format_binary_int24)) ||
        ((obj->interface->type == interface_file) &&
         (obj->format->type == format_binary_int32)) ||
        ((obj->interface->type == interface_uac_out) &&
         (obj->format->type == format_binary_int08)) ||
        ((obj->interface->type == interface_uac_out) &&
         (obj->format->type == format_binary_int16)) ||
        ((obj->interface->type == interface_uac_out) &&
         (obj->format->type == format_binary_int24)) ||
        ((obj->interface->type == interface_uac_out) &&
         (obj->format->type == format_binary_int32)) ||
        ((obj->interface->type == interface_socket) &&
         (obj->format->type == format_binary_int08)) ||
        ((obj->interface->type == interface_socket) &&
         (obj->format->type == format_binary_int16)) ||
        ((obj->interface->type == interface_socket) &&
         (obj->format->type == format_binary_int24)) ||
        ((obj->interface->type == interface_socket) &&
         (obj->format->type == format_binary_int32)))) {
    printf("Sink hops: Invalid interface and/or format.\n");
    exit(EXIT_FAILURE);
  }

  obj->buffer = (char *)malloc(sizeof(char) * msg_hops_config->nChannels *
                               msg_hops_config->hopSize * 4);
  memset(
      obj->buffer, 0x00,
      sizeof(char) * msg_hops_config->nChannels * msg_hops_config->hopSize * 4);
  obj->bufferSize = 0;

  obj->in = (msg_hops_obj *)NULL;

#ifdef UAC_SRC_SINK
  // UAC sink
  obj->uac_hdl = NULL;
  obj->uac_hw_params = NULL;
  obj->uac_sw_params = NULL;
#else
  obj->uac_out_cb = NULL;
  obj->uac_out_ud = NULL;
#endif

  return obj;
}

void snk_hops_destroy(snk_hops_obj *obj) {
  format_destroy(obj->format);
  interface_destroy(obj->interface);
  free((void *)obj->buffer);

  free((void *)obj);
}

void snk_hops_connect(snk_hops_obj *obj, msg_hops_obj *in) { obj->in = in; }

void snk_hops_disconnect(snk_hops_obj *obj) { obj->in = (msg_hops_obj *)NULL; }

void snk_hops_open(snk_hops_obj *obj) {
  switch (obj->interface->type) {
    case interface_blackhole:

      snk_hops_open_interface_blackhole(obj);

      break;

    case interface_file:

      snk_hops_open_interface_file(obj);

      break;

    case interface_socket:

      snk_hops_open_interface_socket(obj);

      break;

    case interface_uac_out:

      snk_hops_open_interface_uac_out(obj);

      break;

    default:

      printf("Sink hops: Invalid interface type.\n");
      exit(EXIT_FAILURE);

      break;
  }
}

void snk_hops_open_interface_blackhole(snk_hops_obj *obj) {
  // Empty
}

void snk_hops_open_interface_file(snk_hops_obj *obj) {
  obj->fp = fopen(obj->interface->fileName, "wb");

  if (obj->fp == NULL) {
    printf("Cannot open file %s\n", obj->interface->fileName);
    exit(EXIT_FAILURE);
  }
}

void snk_hops_open_interface_socket(snk_hops_obj *obj) {
  memset(&(obj->sserver), 0x00, sizeof(struct sockaddr_in));

  obj->sserver.sin_family = AF_INET;
  obj->sserver.sin_addr.s_addr = inet_addr(obj->interface->ip);
  obj->sserver.sin_port = htons(obj->interface->port);
  obj->sid = socket(AF_INET, SOCK_STREAM, 0);

  if ((connect(obj->sid, (struct sockaddr *)&(obj->sserver),
               sizeof(obj->sserver))) < 0) {
    printf("Sink hops: Cannot connect to server\n");
    exit(EXIT_FAILURE);
  }
}

void snk_hops_open_interface_uac_out(snk_hops_obj *obj) {
#ifdef UAC_SRC_SINK
  int err;
  err = uac_alsa_playback_init(obj);
  if (err != 0) {
    printf("Cannot open UAC device\n");
    exit(EXIT_FAILURE);
  }
#endif
}

void snk_hops_close(snk_hops_obj *obj) {
  switch (obj->interface->type) {
    case interface_blackhole:

      snk_hops_close_interface_blackhole(obj);

      break;

    case interface_file:

      snk_hops_close_interface_file(obj);

      break;

    case interface_socket:

      snk_hops_close_interface_socket(obj);

      break;

    case interface_uac_out:

      snk_hops_close_interface_uac_out(obj);

      break;

    default:

      printf("Sink hops: Invalid interface type.\n");
      exit(EXIT_FAILURE);

      break;
  }
}

void snk_hops_close_interface_blackhole(snk_hops_obj *obj) {
  // Empty
}

void snk_hops_close_interface_file(snk_hops_obj *obj) { fclose(obj->fp); }

void snk_hops_close_interface_socket(snk_hops_obj *obj) { close(obj->sid); }

void snk_hops_close_interface_uac_out(snk_hops_obj *obj) {
#ifdef UAC_SRC_SINK
  uac_alsa_playback_deinit(obj);
#else
  obj->uac_out_cb = NULL;
  obj->uac_out_ud = NULL;
#endif
}

int snk_hops_process(snk_hops_obj *obj) {
  int rtnValue;

  if (obj->in->timeStamp != 0) {
    switch (obj->format->type) {
      case format_binary_int08:

        snk_hops_process_format_binary_int08(obj);

        break;

      case format_binary_int16:

        snk_hops_process_format_binary_int16(obj);

        break;

      case format_binary_int24:

        snk_hops_process_format_binary_int24(obj);

        break;

      case format_binary_int32:

        snk_hops_process_format_binary_int32(obj);

        break;

      case format_undefined:

        snk_hops_process_format_undefined(obj);

        break;

      default:

        printf("Sink hops: Invalid format type.\n");
        exit(EXIT_FAILURE);

        break;
    }

    switch (obj->interface->type) {
      case interface_blackhole:

        snk_hops_process_interface_blackhole(obj);

        break;

      case interface_file:

        snk_hops_process_interface_file(obj);

        break;

      case interface_socket:

        snk_hops_process_interface_socket(obj);

        break;

      case interface_uac_out:

        snk_hops_process_interface_uac_out(obj);

        break;

      default:

        printf("Sink hops: Invalid interface type.\n");
        exit(EXIT_FAILURE);

        break;
    }

    rtnValue = 0;

  } else {
    rtnValue = -1;
  }

  return rtnValue;
}

void snk_hops_process_interface_blackhole(snk_hops_obj *obj) {
  // Emtpy
}

void snk_hops_process_interface_file(snk_hops_obj *obj) {
  fwrite(obj->buffer, sizeof(char), obj->bufferSize, obj->fp);
}

void snk_hops_process_interface_socket(snk_hops_obj *obj) {
  if (send(obj->sid, obj->buffer, obj->bufferSize, 0) < 0) {
    printf("Sink hops: Could not send message.\n");
    exit(EXIT_FAILURE);
  }
}

void snk_hops_process_interface_uac_out(snk_hops_obj *obj) {
#ifdef UAC_SRC_SINK
  // send to UAC
  int err;
  int send_finish_status = 0;

  while (send_finish_status == 0) {
    err = snd_pcm_wait(obj->uac_hdl, PCM_WAIT_TIME_MS);
    if (err == 0) {
      continue;
    }

    err = snd_pcm_writei(obj->uac_hdl, obj->buffer, obj->uac_period_size);
    if (err == -EPIPE) {
      /* EPIPE means underrun */
      printf("[%s:%d] snd_pcm_writei: underrun occurred, err=%d \n",
             __FUNCTION__, __LINE__, err);
      snd_pcm_prepare(obj->uac_hdl);
    } else if (err < 0) {
      printf("[%s:%d] snd_pcm_writei: error from writei: %s\n", __FUNCTION__,
             __LINE__, snd_strerror(err));
      break;
    } else if (err != (int)obj->uac_period_size) {
      printf("[%s:%d] snd_pcm_writei: wrote %d frames\n", __FUNCTION__,
             __LINE__, err);
      break;
    } else {
      send_finish_status = 1;
    }
  }
#else
  if (obj->uac_out_cb != NULL) {
    obj->uac_out_cb(obj->buffer, obj->bufferSize, obj->uac_out_ud);
  }
#endif
}

void snk_hops_process_format_binary_int08(snk_hops_obj *obj) {
  unsigned int iChannel;
  unsigned int iSample;
  float sample;
  unsigned int nBytes;
  unsigned int nBytesTotal;

  nBytes = 1;
  nBytesTotal = 0;

  for (iSample = 0; iSample < obj->hopSize; iSample++) {
    for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {
      sample = obj->in->hops->array[iChannel][iSample];
      pcm_normalized2signedXXbits(sample, nBytes, obj->bytes);
      memcpy(&(obj->buffer[nBytesTotal]), &(obj->bytes[4 - nBytes]),
             sizeof(char) * nBytes);

      nBytesTotal += nBytes;
    }
  }

  obj->bufferSize = nBytesTotal;
}

void snk_hops_process_format_binary_int16(snk_hops_obj *obj) {
  unsigned int iChannel;
  unsigned int iSample;
  float sample;
  unsigned int nBytes;
  unsigned int nBytesTotal;

  nBytes = 2;
  nBytesTotal = 0;

  // test write to file
  // snk_hops_save_to_file(obj->in);

  for (iSample = 0; iSample < obj->hopSize; iSample++) {
    for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {
      sample = obj->in->hops->array[iChannel][iSample];
      pcm_normalized2signedXXbits(sample, nBytes, obj->bytes);
      memcpy(&(obj->buffer[nBytesTotal]), &(obj->bytes[4 - nBytes]),
             sizeof(char) * nBytes);

      nBytesTotal += nBytes;
    }
  }

  obj->bufferSize = nBytesTotal;
}

void snk_hops_process_format_binary_int24(snk_hops_obj *obj) {
  unsigned int iChannel;
  unsigned int iSample;
  float sample;
  unsigned int nBytes;
  unsigned int nBytesTotal;

  nBytes = 3;
  nBytesTotal = 0;

  for (iSample = 0; iSample < obj->hopSize; iSample++) {
    for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {
      sample = obj->in->hops->array[iChannel][iSample];
      pcm_normalized2signedXXbits(sample, nBytes, obj->bytes);
      memcpy(&(obj->buffer[nBytesTotal]), &(obj->bytes[4 - nBytes]),
             sizeof(char) * nBytes);

      nBytesTotal += nBytes;
    }
  }

  obj->bufferSize = nBytesTotal;
}

void snk_hops_process_format_binary_int32(snk_hops_obj *obj) {
  unsigned int iChannel;
  unsigned int iSample;
  float sample;
  unsigned int nBytes;
  unsigned int nBytesTotal;

  nBytes = 4;
  nBytesTotal = 0;

  for (iSample = 0; iSample < obj->hopSize; iSample++) {
    for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {
      sample = obj->in->hops->array[iChannel][iSample];
      pcm_normalized2signedXXbits(sample, nBytes, obj->bytes);
      memcpy(&(obj->buffer[nBytesTotal]), &(obj->bytes[4 - nBytes]),
             sizeof(char) * nBytes);

      nBytesTotal += nBytes;
    }
  }

  obj->bufferSize = nBytesTotal;
}

void snk_hops_process_format_undefined(snk_hops_obj *obj) {
  obj->buffer[0] = 0x00;
  obj->bufferSize = 0;
}

snk_hops_cfg *snk_hops_cfg_construct(void) {
  snk_hops_cfg *cfg;

  cfg = (snk_hops_cfg *)malloc(sizeof(snk_hops_cfg));

  cfg->fS = 0;
  cfg->format = (format_obj *)NULL;
  cfg->interface = (interface_obj *)NULL;

  return cfg;
}

void snk_hops_cfg_destroy(snk_hops_cfg *snk_hops_config) {
  if (snk_hops_config->format != NULL) {
    format_destroy(snk_hops_config->format);
  }
  if (snk_hops_config->interface != NULL) {
    interface_destroy(snk_hops_config->interface);
  }

  free((void *)snk_hops_config);
}
