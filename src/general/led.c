#include "general/led.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define LED_BRIGHTNESS_FILE "/sys/class/leds/led%d/brightness"
#define LED_NUM 36
#define LED_ON "128"
#define LED_OFF "0"

int write_int(char const* path, char const* value);

int led_turn_on_all() {
  char path[64];
  int i;
  for (i = 1; i <= LED_NUM; i++) {
    sprintf(path, LED_BRIGHTNESS_FILE, i);
    if (write_int(path, LED_ON) == -1) {
      return -1;
    }
  }
  return 0;
}

int led_turn_off_all() {
  char path[64];
  int i;
  for (i = 1; i <= LED_NUM; i++) {
    sprintf(path, LED_BRIGHTNESS_FILE, i);
    if (write_int(path, LED_OFF) == -1) {
      return -1;
    }
  }
  return 0;
}

int led_turn_on_idx(int idx, int r, int g, int b) {
  char path[64];
  for (int i = 1; i <= 3; i++) {
    memset(path, 0, sizeof(path));
    sprintf(path, LED_BRIGHTNESS_FILE, 3 * idx + i);
    char value[1] = {0};
    if (i == 1) {  // green
      sprintf(value, "%d", g);
    } else if (i == 2) {  // red
      sprintf(value, "%d", r);
    } else {  // blue
      sprintf(value, "%d", b);
    }
    if (write_int(path, value) == -1) {
      return -1;
    }
  }

  return 0;
}

int led_turn_on_mic_idx(int idx, int r, int g, int b) {
  char path[64];
  int mic_idx = idx * 2;
  for (int i = 1; i <= 3; i++) {
    memset(path, 0, sizeof(path));
    int led_idx = 3 * mic_idx + i;
    sprintf(path, LED_BRIGHTNESS_FILE, led_idx);
    char value[1] = {0};
    if (i == 1) {  // green
      sprintf(value, "%d", g);
    } else if (i == 2) {  // red
      sprintf(value, "%d", r);
    } else {  // blue
      sprintf(value, "%d", b);
    }
    if (write_int(path, value) == -1) {
      return -1;
    }
  }

  return 0;
}

int write_int(char const* path, char const* value) {
  int fd;
  char read_buffer[64], write_buffer[64];
  ssize_t length_r, length_w;

  static int already_warned = 0;

  fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd >= 0) {
    char buffer[128];
    int bytes;
    length_r = read(fd, buffer, 64);
    printf("The current value of %s is : %d\n", path, atoi(buffer));
    if (!memcmp(buffer, value, length_r)) {
      // don't need to set value, because
      // the value is same with the target value
      close(fd);
      return 0;
    }
    bytes = strlen(value) + 1;  // include the NULL terminator
    length_w = write(fd, value, bytes);
    printf("%d bytes is wrote to node %s, the current value is %s\n", length_w,
           path, value);

    close(fd);
    return length_w == -1 ? -1 : 0;
  } else {
    fprintf(stderr, "can't open led driver file,%s\n", path);
    if (already_warned == 0) {
      fprintf(stderr, "write_int failed to open led driver %s\n", path);
      already_warned = 1;
    }
    return -1;
  }
}