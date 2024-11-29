/**
 * \file     src_hops.c
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

#include <source/src_hops.h>

static FILE *test_input_file = NULL;

static void src_hops_save_to_file(src_hops_obj *obj) {
  if (test_input_file == NULL) {
    test_input_file = fopen("/home/meonardo/bin/input.pcm", "w+");
    if (test_input_file == NULL) {
      printf("Cannot open file input.pcm\n");
      exit(EXIT_FAILURE);
    }
  }

  fwrite(obj->buffer, 1, obj->bufferSize, test_input_file);
}

static void src_hops_save_to_file1(void *buffer, size_t len) {
  if (test_input_file == NULL) {
    test_input_file = fopen("/home/meonardo/bin/input.pcm", "w+");
    if (test_input_file == NULL) {
      printf("Cannot open file input.pcm\n");
      exit(EXIT_FAILURE);
    }
  }

  fwrite(buffer, 1, len, test_input_file);
}

src_hops_obj *src_hops_construct(const src_hops_cfg *src_hops_config,
                                 const msg_hops_cfg *msg_hops_config) {
  src_hops_obj *obj;
  unsigned int nBytes;

  obj = (src_hops_obj *)malloc(sizeof(src_hops_obj));

  obj->timeStamp = 0;

  obj->hopSize = msg_hops_config->hopSize;
  obj->nChannels = msg_hops_config->nChannels;
  obj->fS = msg_hops_config->fS;

  obj->format = format_clone(src_hops_config->format);
  obj->interface = interface_clone(src_hops_config->interface);
  if (src_hops_config->channel_map != NULL) {
    // Will not be null if in pulseaudio mode
    memcpy(&obj->cm, src_hops_config->channel_map, sizeof(pa_channel_map));
  } else if (obj->interface->type == interface_pulseaudio) {
    // Can't be null if we are in pulseaudio mode
    printf("Error: Pulseaudio interface requires channel map.\n");
    exit(EXIT_FAILURE);
  }

  memset(obj->bytes, 0x00, 4 * sizeof(char));

  if (!((obj->interface->type == interface_file ||
         obj->interface->type == interface_soundcard ||
         obj->interface->type == interface_pulseaudio ||
         obj->interface->type == interface_customized_pcm ||
         obj->interface->type == interface_socket) &&
        (obj->format->type == format_binary_int08 ||
         obj->format->type == format_binary_int16 ||
         obj->format->type == format_binary_int24 ||
         obj->format->type == format_binary_int32))) {
    printf("Source hops: Invalid interface and/or format.\n");
    exit(EXIT_FAILURE);
  }

  obj->buffer =
      (char *)malloc(sizeof(char) * obj->hopSize * obj->nChannels * 4);
  memset(obj->buffer, 0x00, sizeof(char) * obj->hopSize * obj->nChannels * 4);

  switch (obj->format->type) {
    case format_binary_int08:
      obj->bufferSize = obj->hopSize * obj->nChannels * 1;
      break;
    case format_binary_int16:
      obj->bufferSize = obj->hopSize * obj->nChannels * 2;
      break;
    case format_binary_int24:
      obj->bufferSize = obj->hopSize * obj->nChannels * 3;
      break;
    case format_binary_int32:
      obj->bufferSize = obj->hopSize * obj->nChannels * 4;
      break;
  }

  obj->out = (msg_hops_obj *)NULL;

  return obj;
}

void src_hops_destroy(src_hops_obj *obj) {
  free((void *)obj->buffer);
  format_destroy(obj->format);
  interface_destroy(obj->interface);

  free((void *)obj);
}

void src_hops_connect(src_hops_obj *obj, msg_hops_obj *out) { obj->out = out; }

void src_hops_disconnect(src_hops_obj *obj) { obj->out = (msg_hops_obj *)NULL; }

void src_hops_open(src_hops_obj *obj) {
  switch (obj->interface->type) {
    case interface_file:

      src_hops_open_interface_file(obj);

      break;

    case interface_soundcard:

      src_hops_open_interface_soundcard(obj);

      break;

    case interface_pulseaudio:

      src_hops_open_interface_pulseaudio(obj);

      break;
    case interface_customized_pcm:

      src_hops_open_interface_customized_pcm(obj);

      break;

    case interface_socket:

      src_hops_open_interface_socket(obj);

      break;

    default:

      printf("Source hops: Invalid interface type.\n");
      exit(EXIT_FAILURE);

      break;
  }
}

void src_hops_open_interface_file(src_hops_obj *obj) {
  obj->fp = fopen(obj->interface->fileName, "rb");

  if (obj->fp == NULL) {
    printf("Cannot open file %s\n", obj->interface->fileName);
    exit(EXIT_FAILURE);
  }
}

void src_hops_open_interface_soundcard(src_hops_obj *obj) {
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_format_t format;
  int err;

  switch (obj->format->type) {
    case format_binary_int08:

      format = SND_PCM_FORMAT_S8;

      break;

    case format_binary_int16:

      format = SND_PCM_FORMAT_S16_LE;

      break;

    case format_binary_int24:

      format = SND_PCM_FORMAT_S24_LE;

      break;

    case format_binary_int32:

      format = SND_PCM_FORMAT_S32_LE;

      break;

    default:

      printf("Source hops: Invalid format.\n");
      exit(EXIT_FAILURE);

      break;
  }

  if ((err = snd_pcm_open(&(obj->ch), obj->interface->deviceName,
                          SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    printf("Source hops: Cannot open audio device %s: %s\n",
           obj->interface->deviceName, snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    printf("Source hops: Cannot allocate hardware parameter structure: %s\n",
           snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  if ((err = snd_pcm_hw_params_any(obj->ch, hw_params)) < 0) {
    printf("Source hops: Cannot initialize hardware parameter structure: %s\n",
           snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  if ((err = snd_pcm_hw_params_set_access(obj->ch, hw_params,
                                          SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    printf("Source hops: Cannot set access type: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  if ((err = snd_pcm_hw_params_set_format(obj->ch, hw_params, format)) < 0) {
    printf("Source hops: Cannot set sample format: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  if ((err = snd_pcm_hw_params_set_rate(obj->ch, hw_params, obj->fS, 0)) < 0) {
    printf("Source hops: Cannot set sample rate: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  if ((err = snd_pcm_hw_params_set_channels(obj->ch, hw_params,
                                            obj->nChannels)) < 0) {
    printf("Source hops: Cannot set channel count: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  if ((err = snd_pcm_hw_params(obj->ch, hw_params)) < 0) {
    printf("Source hops: Cannot set parameters: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  snd_pcm_hw_params_free(hw_params);

  if ((err = snd_pcm_prepare(obj->ch)) < 0) {
    printf("Source hops: Cannot prepare audio interface for use: %s\n",
           snd_strerror(err));
    exit(EXIT_FAILURE);
  }
}

void src_hops_open_interface_pulseaudio(src_hops_obj *obj) {
  pa_sample_format_t format;

  switch (obj->format->type) {
    case format_binary_int08:

      format = PA_SAMPLE_U8;

      break;

    case format_binary_int16:

      format = PA_SAMPLE_S16LE;

      break;

    case format_binary_int24:

      format = PA_SAMPLE_S24LE;

      break;

    case format_binary_int32:

      format = PA_SAMPLE_S32LE;

      break;

    default:

      printf("Source hops: Invalid format.\n");
      exit(EXIT_FAILURE);

      break;
  }

  obj->ss.format = format;
  obj->ss.rate = obj->fS;
  obj->ss.channels = obj->nChannels;

  int err;
  if (!(obj->pa = pa_simple_new(NULL, "Odas", PA_STREAM_RECORD,
                                obj->interface->deviceName, "record", &obj->ss,
                                &obj->cm, NULL, &err))) {
    printf("Source hops: Cannot open pulseaudio device %s: %s\n",
           obj->interface->deviceName, pa_strerror(err));
    exit(EXIT_FAILURE);
  }
}

void src_hops_open_interface_customized_pcm(src_hops_obj *obj) {
  obj->sps.ai_dev = SAMPLE_AUDIO_INNER_AI_DEV;
  obj->sps.chn = obj->nChannels;
  obj->sps.sample_rate = obj->fS;
  obj->sps.per_frame = obj->hopSize;
  obj->sps.init = TD_FALSE;

  switch (obj->format->type) {
    case format_binary_int08:
      obj->sps.bit_width = OT_AUDIO_BIT_WIDTH_8;
      break;
    case format_binary_int16:
      obj->sps.bit_width = OT_AUDIO_BIT_WIDTH_16;
      break;
    case format_binary_int24:
      obj->sps.bit_width = OT_AUDIO_BIT_WIDTH_24;
      break;
    default:
      printf("%s,%d Source hops: Invalid format for PCM source.\n",
             __FUNCTION__, __LINE__);
      break;
  }

  if (src_pcm_init(&(obj->sps)) == TD_FAILURE) {
    printf("%s,%d Source hops: Cannot init customized PCM source.\n",
           __FUNCTION__, __LINE__);
    return;
  }
}

void src_hops_open_interface_socket(src_hops_obj *obj) {
  memset(&(obj->sserver), 0x00, sizeof(struct sockaddr_in));

  obj->sserver.sin_family = AF_INET;
  obj->sserver.sin_addr.s_addr = inet_addr(obj->interface->ip);
  obj->sserver.sin_port = htons(obj->interface->port);
  obj->sid = socket(AF_INET, SOCK_STREAM, 0);

  if ((connect(obj->sid, (struct sockaddr *)&(obj->sserver),
               sizeof(obj->sserver))) < 0) {
    printf("Source hops: Cannot connect to server\n");
    exit(EXIT_FAILURE);
  }
}

void src_hops_close(src_hops_obj *obj) {
  switch (obj->interface->type) {
    case interface_file:

      src_hops_close_interface_file(obj);

      break;

    case interface_soundcard:

      src_hops_close_interface_soundcard(obj);

      break;

    case interface_pulseaudio:

      src_hops_close_interface_pulseaudio(obj);

      break;

    case interface_customized_pcm:

      src_hops_close_interface_customized_pcm(obj);

      break;

    case interface_socket:

      src_hops_close_interface_socket(obj);

      break;

    default:

      printf("Source hops: Invalid interface type.\n");
      exit(EXIT_FAILURE);

      break;
  }
}

void src_hops_close_interface_file(src_hops_obj *obj) { fclose(obj->fp); }

void src_hops_close_interface_soundcard(src_hops_obj *obj) {
  snd_pcm_close(obj->ch);
}

void src_hops_close_interface_pulseaudio(src_hops_obj *obj) {
  if (obj->pa != NULL) pa_simple_free(obj->pa);
}

void src_hops_close_interface_customized_pcm(src_hops_obj *obj) {
  if (obj->pa != NULL) pa_simple_free(obj->pa);
}

void src_hops_close_interface_socket(src_hops_obj *obj) { close(obj->sid); }

int src_hops_process(src_hops_obj *obj) {
  int rtnValue;

  switch (obj->format->type) {
    case format_binary_int08:

      src_hops_process_format_binary_int08(obj);

      break;

    case format_binary_int16:

      src_hops_process_format_binary_int16(obj);

      break;

    case format_binary_int24:

      src_hops_process_format_binary_int24(obj);

      break;

    case format_binary_int32:

      src_hops_process_format_binary_int32(obj);

      break;

    default:

      printf("Source hops: Invalid format type.\n");
      exit(EXIT_FAILURE);

      break;
  }

  switch (obj->interface->type) {
    case interface_file:

      rtnValue = src_hops_process_interface_file(obj);

      break;

    case interface_soundcard:

      rtnValue = src_hops_process_interface_soundcard(obj);

      break;

    case interface_pulseaudio:

      rtnValue = src_hops_process_interface_pulseaudio(obj);

      break;

    case interface_customized_pcm:

      rtnValue = src_hops_process_interface_customized_pcm(obj);

      break;

    case interface_socket:

      rtnValue = src_hops_process_interface_socket(obj);

      break;

    default:

      printf("Source hops: Invalid interface type.\n");
      exit(EXIT_FAILURE);

      break;
  }

  obj->timeStamp++;
  obj->out->timeStamp = obj->timeStamp;

  return rtnValue;
}

int src_hops_process_interface_file(src_hops_obj *obj) {
  unsigned int nBytesTotal;
  int rtnValue;

  nBytesTotal = fread(obj->buffer, sizeof(char), obj->bufferSize, obj->fp);

  if (nBytesTotal == obj->bufferSize) {
    rtnValue = 0;

  } else {
    rtnValue = -1;
  }

  return rtnValue;
}

int src_hops_process_interface_soundcard(src_hops_obj *obj) {
  unsigned int nSamples;
  int rtnValue;
  int err;

  if (err = snd_pcm_readi(obj->ch, obj->buffer, obj->hopSize) > 0) {
    rtnValue = 0;

  } else {
    rtnValue = -1;
  }

  return rtnValue;
}

int src_hops_process_interface_pulseaudio(src_hops_obj *obj) {
  int rtnValue;
  int err;

  if (pa_simple_read(obj->pa, obj->buffer, obj->bufferSize, &err) < 0) {
    rtnValue = -1;

  } else {
    rtnValue = 0;
  }

  return rtnValue;
}

int src_hops_process_interface_customized_pcm(src_hops_obj *obj) {
  int offset = 0;
  int size_per_sample = obj->format->type / 8;
  int frame_len = obj->hopSize;

  // read frame for each channel
  for (int i = 0; i < obj->sps.chn; i++) {
    src_pcm_item *item = &obj->sps.items[i];
    if (src_pcm_read_frame(&obj->sps, item) != TD_SUCCESS) {
      printf("Cannot read frame for channel %d\n", i);
      return -1;  // WARN: memory leak, not release the read frames
    }
  }

  // copy each channel data to buffer
  // the memory layout of the `buffer` will be interleaved
  // for example of 8 channels and 480 samples
  // [ch1 sample1] [ch2 sample1] ... [ch8 sample1]
  // [ch1 sample2] [ch2 sample2] ... [ch8 sample2]
  // ...
  // [ch1 sample480] [ch2 sample480] ... [ch8 sample480]
  for (int i = 0; i < frame_len; i++) {
    for (int j = 0; j < obj->sps.chn; j++) {
      src_pcm_item *item = &obj->sps.items[j];
      td_u8 *data = item->frame.virt_addr[0];
      offset = i * obj->sps.chn * size_per_sample + j * size_per_sample;
      memcpy(obj->buffer + offset, data + i * size_per_sample, size_per_sample);
    }
  }

  // release frames
  for (int i = 0; i < obj->sps.chn; i++) {
    src_pcm_item *item = &obj->sps.items[i];
    src_pcm_release_frame(&obj->sps, item);
  }

  // write to file
  src_hops_save_to_file(obj);

  return 0;
}

int src_hops_process_interface_socket(src_hops_obj *obj) {
  unsigned int nBytesTotal;
  int rtnValue;

  // ssize_t recv(int sockfd, void *buf, size_t len, int flags);
  nBytesTotal = recv(obj->sid, obj->buffer, obj->bufferSize, MSG_WAITALL);

  if (nBytesTotal == obj->bufferSize) {
    rtnValue = 0;

  } else {
    rtnValue = -1;
  }

  return rtnValue;
}

void src_hops_process_format_binary_int08(src_hops_obj *obj) {
  unsigned int iSample;
  unsigned int iChannel;
  unsigned int nBytes;
  float sample;

  nBytes = 1;

  for (iSample = 0; iSample < obj->hopSize; iSample++) {
    for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {
      memcpy(&(obj->bytes[4 - nBytes]),
             &(obj->buffer[(iSample * obj->nChannels + iChannel) * nBytes]),
             sizeof(char) * nBytes);

      sample = pcm_signedXXbits2normalized(obj->bytes, nBytes);

      obj->out->hops->array[iChannel][iSample] = sample;
    }
  }
}

void src_hops_process_format_binary_int16(src_hops_obj *obj) {
  unsigned int iSample;
  unsigned int iChannel;
  unsigned int nBytes;
  float sample;

  nBytes = 2;

  for (iSample = 0; iSample < obj->hopSize; iSample++) {
    for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {
      memcpy(&(obj->bytes[4 - nBytes]),
             &(obj->buffer[(iSample * obj->nChannels + iChannel) * nBytes]),
             sizeof(char) * nBytes);

      sample = pcm_signedXXbits2normalized(obj->bytes, nBytes);

      obj->out->hops->array[iChannel][iSample] = sample;
    }
  }
}

void src_hops_process_format_binary_int24(src_hops_obj *obj) {
  unsigned int iSample;
  unsigned int iChannel;
  unsigned int nBytes;
  float sample;

  nBytes = 3;

  for (iSample = 0; iSample < obj->hopSize; iSample++) {
    for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {
      memcpy(&(obj->bytes[4 - nBytes]),
             &(obj->buffer[(iSample * obj->nChannels + iChannel) * nBytes]),
             sizeof(char) * nBytes);

      sample = pcm_signedXXbits2normalized(obj->bytes, nBytes);

      obj->out->hops->array[iChannel][iSample] = sample;
    }
  }
}

void src_hops_process_format_binary_int32(src_hops_obj *obj) {
  unsigned int iSample;
  unsigned int iChannel;
  unsigned int nBytes;
  float sample;

  nBytes = 4;

  for (iSample = 0; iSample < obj->hopSize; iSample++) {
    for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {
      memcpy(&(obj->bytes[4 - nBytes]),
             &(obj->buffer[(iSample * obj->nChannels + iChannel) * nBytes]),
             sizeof(char) * nBytes);

      sample = pcm_signedXXbits2normalized(obj->bytes, nBytes);

      obj->out->hops->array[iChannel][iSample] = sample;
    }
  }
}

src_hops_cfg *src_hops_cfg_construct(void) {
  src_hops_cfg *cfg;

  cfg = (src_hops_cfg *)malloc(sizeof(src_hops_cfg));

  cfg->format = (format_obj *)NULL;
  cfg->interface = (interface_obj *)NULL;

  return cfg;
}

void src_hops_cfg_destroy(src_hops_cfg *src_hops_config) {
  if (src_hops_config->format != NULL) {
    format_destroy(src_hops_config->format);
  }
  if (src_hops_config->interface != NULL) {
    interface_destroy(src_hops_config->interface);
  }
  if (src_hops_config->channel_map != NULL) {
    free((void *)src_hops_config->channel_map);
  }

  free((void *)src_hops_config);
}
