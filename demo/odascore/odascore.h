#ifndef ODAS_CORE_H
#define ODAS_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_FILEPATH_LEN 256

#ifdef __cplusplus
extern "C" {
#endif /* end of #ifdef __cplusplus */

typedef struct objects objects_t;
typedef struct aobjects aobjects_t;
typedef struct configs configs_t;
typedef struct profiler profiler_t;

typedef void (*odascore_callback)(const char *buf, size_t len, void *user_data);

typedef struct odascore {
  // configuration file path
  char config_filepath[MAX_FILEPATH_LEN];
  // for single-threaded processing objs
  objects_t *objs;
  // for multi-threaded processing aobjs
  aobjects_t *aobjs;
  // configuration objs
  configs_t *cfgs;
  // profiler objs
  profiler_t *prf;
  // whether to use single or multi-threaded processing, default is multi-threaded processing
  // 0 for multi-threaded processing, 1 for single-threaded processing
  int thread_type;

  // callback function
  odascore_callback read_callback;
  void* user_data;
  
  // worker thread
  pthread_t worker;
} odascore_t;

/// @brief Construct odascore object
/// @param config_filepath odas configuration file path
/// @param type 0 for multi-threaded processing, 1 for single-threaded processing
/// @param read_callback SSS result buffer callback
/// @param user_data user data
/// @return odascore object
odascore_t *odascore_construct(const char *config_filepath, int type,
                               odascore_callback read_callback, void* user_data);

/// @brief Destroy odascore object
/// @param core odascore object
void odascore_destroy(odascore_t *core);

/// @brief Feed frames to odascore object
/// @param core  odascore object
/// @param chn_bufs channel buffers [chn_buf 0, chn_buf 1, ...]
/// @param n_chns number of channels
/// @return 0 for success, -1 for failure
int odascore_put_frames(odascore_t *core, char **chn_bufs, int n_chns);

#ifdef __cplusplus
}
#endif /* end of #ifdef __cplusplus */

#endif  // ODAS_CORE_H