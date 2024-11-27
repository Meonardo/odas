
#include <alsa/asoundlib.h>
#include <getopt.h>
#include <odas/odas.h>
#include <signal.h>
#include <time.h>

#include "configs.h"
#include "objects.h"
#include "parameters.h"
#include "profiler.h"
#include "threads.h"

static void list_alsa_cards_devices() {
  int card = -1;  // Card index
  int err;

  printf("Listing ALSA sound cards and devices:\n");

  // Iterate through sound cards
  while (snd_card_next(&card) >= 0 && card >= 0) {
    printf("Sound Card #%d:\n", card);

    char card_name[32];
    char card_longname[80];

    // Get card short and long names
    snd_card_get_name(card, &card_name[0]);
    snd_card_get_longname(card, &card_longname[0]);

    printf("  Short Name: %s\n", card_name);
    printf("  Long Name: %s\n", card_longname);

    // Open control interface for this card
    snd_ctl_t *ctl;
    char ctl_name[32];
    sprintf(ctl_name, "hw:%d", card);

    if ((err = snd_ctl_open(&ctl, ctl_name, 0)) < 0) {
      fprintf(stderr, "Error opening control interface for card %d: %s\n", card,
              snd_strerror(err));
      continue;
    }

    // Enumerate PCM devices
    int device = -1;
    while (snd_ctl_pcm_next_device(ctl, &device) >= 0 && device >= 0) {
      printf("    PCM Device #%d:\n", device);

      snd_pcm_info_t *pcm_info;
      snd_pcm_info_alloca(&pcm_info);

      snd_pcm_info_set_device(pcm_info, device);
      snd_pcm_info_set_subdevice(pcm_info, 0);  // Default subdevice
      snd_pcm_info_set_stream(pcm_info,
                              SND_PCM_STREAM_CAPTURE);  // For capture devices

      if ((err = snd_ctl_pcm_info(ctl, pcm_info)) < 0) {
        printf("      No capture information: %s\n", snd_strerror(err));
        continue;
      }

      printf("      Device Name: %s\n", snd_pcm_info_get_name(pcm_info));
      printf("      Device ID: %s\n", snd_pcm_info_get_id(pcm_info));
      printf("      Subdevices: %d/%d\n",
             snd_pcm_info_get_subdevices_avail(pcm_info),
             snd_pcm_info_get_subdevices_count(pcm_info));
    }

    snd_ctl_close(ctl);
  }
}

// +----------------------------------------------------------+
// | Variables                                                |
// +----------------------------------------------------------+

// +------------------------------------------------------+
// | Type                                                 |
// +------------------------------------------------------+

enum processing { processing_singlethread, processing_multithread } type;

// +------------------------------------------------------+
// | Getopt                                               |
// +------------------------------------------------------+

int c;
char *file_config;

// +------------------------------------------------------+
// | Objects                                              |
// +------------------------------------------------------+

objects *objs;
aobjects *aobjs;

// +------------------------------------------------------+
// | Configurations                                       |
// +------------------------------------------------------+

configs *cfgs;

// +------------------------------------------------------+
// | Profiler                                             |
// +------------------------------------------------------+

profiler *prf;

// +------------------------------------------------------+
// | Flag                                                 |
// +------------------------------------------------------+

char stopProcess;

// +----------------------------------------------------------+
// | Signal handler                                           |
// +----------------------------------------------------------+

void sighandler(int signum) {
  if (type == processing_singlethread) {
    stopProcess = 1;
  }
  if (type == processing_multithread) {
    threads_multiple_stop(aobjs);
  }
}

// +----------------------------------------------------------+
// | Main routine                                             |
// +----------------------------------------------------------+

int main(int argc, char *argv[]) {
  // list local cards and devices
  list_alsa_cards_devices();

  // +------------------------------------------------------+
  // | Arguments                                            |
  // +------------------------------------------------------+

  file_config = (char *)NULL;
  char verbose = 0x00;

  type = processing_multithread;

  while ((c = getopt(argc, argv, "c:hsv")) != -1) {
    switch (c) {
      case 'c':

        file_config = (char *)malloc(sizeof(char) * (strlen(optarg) + 1));
        strcpy(file_config, optarg);

        break;

      case 'h':

        printf("+----------------------------------------------------+\n");
        printf("|        ODAS (Open embeddeD Audition System)        |\n");
        printf("+----------------------------------------------------+\n");
        printf("| Author:      Francois Grondin                      |\n");
        printf("| Email:       francois.grondin2@usherbrooke.ca      |\n");
        printf("| Website:     introlab.3it.usherbrooke.ca           |\n");
        printf("| Repository:  github.com/introlab/odas              |\n");
        printf("| Version:     1.0                                   |\n");
        printf("+----------------------------------------------------+\n");
        printf("| -c       Configuration file (.cfg)                 |\n");
        printf("| -h       Help                                      |\n");
        printf("| -s       Process sequentially (no multithread)     |\n");
        printf("| -v       Verbose                                   |\n");
        printf("+----------------------------------------------------+\n");

        exit(EXIT_SUCCESS);

        break;

      case 's':

        type = processing_singlethread;

        break;

      case 'v':

        verbose = 0x01;

        break;
    }
  }

  if (file_config == NULL) {
    printf("Missing configuration file.\n");
    exit(EXIT_FAILURE);
  }

  // +------------------------------------------------------+
  // | Copyright                                            |
  // +------------------------------------------------------+

  if (verbose == 0x01) {
    printf("+--------------------------------------------+\n");
    printf("|    ODAS (Open embeddeD Audition System)    |\n");
    printf("+--------------------------------------------+\n");
    printf("| Author:  Francois Grondin                  |\n");
    printf("| Email:   francois.grondin2@usherbrooke.ca  |\n");
    printf("| Website: introlab.3it.usherbrooke.ca       |\n");
    printf("| Version: 1.0                               |\n");
    printf("+--------------------------------------------+\n");
  }

  // +------------------------------------------------------+
  // | Single thread                                        |
  // +------------------------------------------------------+

  if (type == processing_singlethread) {
    // +--------------------------------------------------+
    // | Profiler                                         |
    // +--------------------------------------------------+

    prf = profiler_construct();

    // +--------------------------------------------------+
    // | Configure                                        |
    // +--------------------------------------------------+

    if (verbose == 0x01) printf("| + Initializing configurations...... ");
    fflush(stdout);

    cfgs = configs_construct(file_config);

    if (verbose == 0x01) printf("[Done] |\n");

    // +--------------------------------------------------+
    // | Construct                                        |
    // +--------------------------------------------------+

    if (verbose == 0x01) printf("| + Initializing objects............. ");
    fflush(stdout);

    objs = objects_construct(cfgs);

    if (verbose == 0x01) printf("[Done] |\n");

    // +--------------------------------------------------+
    // | Processing                                       |
    // +--------------------------------------------------+

    if (verbose == 0x01) printf("| + Processing....................... ");
    fflush(stdout);

    threads_single_open(objs);
    stopProcess = 0;
    while ((threads_single_process(objs, prf) == 0) && (stopProcess == 0));
    threads_single_close(objs);

    if (verbose == 0x01) printf("[Done] |\n");

    // +--------------------------------------------------+
    // | Free memory                                      |
    // +--------------------------------------------------+

    if (verbose == 0x01) printf("| + Free memory...................... ");
    fflush(stdout);

    objects_destroy(objs);
    configs_destroy(cfgs);
    free((void *)file_config);

    if (verbose == 0x01) printf("[Done] |\n");

    // +--------------------------------------------------+
    // | Results                                          |
    // +--------------------------------------------------+

    if (verbose == 0x01) profiler_printf(prf);

    profiler_destroy(prf);
  }

  // +------------------------------------------------------+
  // | Multiple threads                                     |
  // +------------------------------------------------------+

  if (type == processing_multithread) {
    // +--------------------------------------------------+
    // | Configure                                        |
    // +--------------------------------------------------+

    if (verbose == 0x01) printf("| + Initializing configurations...... ");
    fflush(stdout);

    cfgs = configs_construct(file_config);

    if (verbose == 0x01) printf("[Done] |\n");

    // +--------------------------------------------------+
    // | Construct                                        |
    // +--------------------------------------------------+

    if (verbose == 0x01) printf("| + Initializing objects............. ");
    fflush(stdout);

    aobjs = aobjects_construct(cfgs);

    if (verbose == 0x01) printf("[Done] |\n");

    // +--------------------------------------------------+
    // | Launch threads                                   |
    // +--------------------------------------------------+

    signal(SIGINT, sighandler);

    if (verbose == 0x01) printf("| + Launch threads................... ");
    fflush(stdout);

    threads_multiple_start(aobjs);

    if (verbose == 0x01) printf("[Done] |\n");

    // +--------------------------------------------------+
    // | Wait                                             |
    // +--------------------------------------------------+

    if (verbose == 0x01) printf("| + Threads running.................. ");
    fflush(stdout);

    threads_multiple_join(aobjs);

    if (verbose == 0x01) printf("[Done] |\n");

    // +--------------------------------------------------+
    // | Free memory                                      |
    // +--------------------------------------------------+

    if (verbose == 0x01) printf("| + Free memory...................... ");
    fflush(stdout);

    aobjects_destroy(aobjs);
    configs_destroy(cfgs);
    free((void *)file_config);

    if (verbose == 0x01) printf("[Done] |\n");

    if (verbose == 0x01)
      printf("+--------------------------------------------+\n");
  }

  return 0;
}
