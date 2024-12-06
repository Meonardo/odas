#include "source/src_pcm.h"

int src_pcm_init(src_pcm_spec* obj) {
  if (obj->init == 1) {
    printf("[%s] src_pcm_spec already init!\n", __FUNCTION__);
    return 0;
  }

  int ret;

  // create a ring buffer
  obj->rb = (ring_buffer_t*)malloc(sizeof(ring_buffer_t));
  size_t buffer_size = obj->per_frame * obj->chn * obj->frame_size;
  obj->buffer = (char*)malloc(buffer_size);
  ret = ring_buffer_init(obj->rb, buffer_size * RB_COUNT);
  if (ret != 0) {
    printf("[%s] ring_buffer_init failed with %d!\n", __FUNCTION__, ret);
    return ret;
  }

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

  // free resources
  if (obj->rb != NULL) {
    ring_buffer_release(obj->rb);
    free(obj->rb);
    free(obj->buffer);
    obj->buffer = NULL;
  }

  printf("[%s] src_pcm_spec destory success!\n\n\n", __FUNCTION__);
}

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