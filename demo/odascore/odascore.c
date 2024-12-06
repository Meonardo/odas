#include "odascore.h"

#include <odas/odas.h>
#include <time.h>

#include "../odaslive/configs.h"
#include "../odaslive/objects.h"
#include "../odaslive/parameters.h"
#include "../odaslive/profiler.h"
#include "../odaslive/threads.h"

char stopProcess;

// single-threaded processing
static int launch_single_thread(odascore_t *core) {
  printf("Single-threaded processing.\n");

  core->prf = profiler_construct();
  // load configurations from file
  core->cfgs = configs_construct(core->config_filepath);
  // construct objects
  core->objs = objects_construct(core->cfgs);

  // assign callback to the `UAC out` sink
  core->objs->snk_hops_seps_vol_object->uac_out_cb = core->read_callback;
  core->objs->snk_hops_seps_vol_object->uac_out_ud = core->user_data;

  // open threads
  threads_single_open(core->objs);
  stopProcess = 0;
  while ((threads_single_process(core->objs, core->prf) == 0) && (stopProcess == 0));

  printf("Single-threaded Processing done.\n");

  return 0;
}

// multi-threaded processing
static int launch_multi_thread(odascore_t *core) {
  printf("Multi-threaded processing.\n");

  core->prf = profiler_construct();
  // load configurations from file
  core->cfgs = configs_construct(core->config_filepath);
  // construct objects
  core->aobjs = aobjects_construct(core->cfgs);

  // assign callback to the `UAC out` sink
  core->aobjs->asnk_hops_seps_vol_object->snk_hops->uac_out_cb =
      core->read_callback;
  core->aobjs->asnk_hops_seps_vol_object->snk_hops->uac_out_ud =
      core->user_data;

  threads_multiple_start(core->aobjs);

  printf("Multi-threaded Processing done.\n");

  return 0;
}

static void destory_single_thread(odascore_t *core) {
  threads_single_close(core->objs);
  // free resources
  aobjects_destroy(core->aobjs);
  configs_destroy(core->cfgs);
}

static void destroy_multi_thread(odascore_t *core) {
  threads_multiple_stop(core->aobjs);
  // free resources
  threads_multiple_join(core->aobjs);
  aobjects_destroy(core->aobjs);
  configs_destroy(core->cfgs);
}

// main loop
static void *run_main_worker(void *args) {
  odascore_t *core = (odascore_t *)args;
  if (core->thread_type == 0) {
    launch_multi_thread(core);
  } else {
    launch_single_thread(core);
  }

  printf("Worker thread done.\n");

  return NULL;
}

odascore_t *odascore_construct(const char *config_filepath, int type,
                               odascore_callback read_callback,
                               void *user_data) {
  if (config_filepath == NULL) {
    printf("Configuration file path is NULL.\n");
    assert(0);
  }
  if (type > 1 || type < 0) {
    printf("Invalid thread type.\n");
    assert(0);
  }

  odascore_t *core;

  core = (odascore_t *)malloc(sizeof(odascore_t));
  memset(core, 0x00, sizeof(odascore_t));

  strcpy(core->config_filepath, config_filepath);
  core->thread_type = type;
  core->read_callback = read_callback;
  core->user_data = user_data;

  // launch worker thread
  pthread_create(&core->worker, NULL, run_main_worker, core);

  return core;
}

void odascore_destroy(odascore_t *core) {
  // stop processing
  if (core->thread_type == 0) {
    stopProcess = 1;
    destory_single_thread(core);
  } else {
    destroy_multi_thread(core);
  }
  // stop worker thread
  pthread_join(core->worker, NULL);
  free(core);
}

int odascore_put_frames(odascore_t *core, char **chn_bufs, int n_chns) {
  if (core->objs != NULL) {
    return src_pcm_write_buffer(&core->objs->src_hops_mics_object->sps,
                                chn_bufs, n_chns);
  } else if (core->aobjs != NULL) {
    return src_pcm_write_buffer(
        &core->aobjs->asrc_hops_mics_object->src_hops->sps, chn_bufs, n_chns);
  }
  return -1;
}