#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define MSG_SIZE 4
#define TRUE 1

static struct Measures {
  int16_t * humidity;
  int16_t * temperature;
};

int main(int argv, int **argc) {
  int fd, ret;
  char msg[MSG_SIZE] = {0,0,0,0};
  struct Measures measures;
  FILE * fic;

  fd = open("/dev/rtdm/rtdm_DHT11_0", O_RDWR);
  if (fd < 0) {
    printf("Open error %d\n", fd);
    exit(fd);
  }
  measures.humidity = &msg[0];
  measures.temperature = &msg[2];
  while(TRUE) {
    ret = read(fd, msg, sizeof(msg));
    if (ret <= 0) {
      printf("%d\n", msg[0]);
      printf("Read error %d\n", ret);
      exit(ret);
    }
    printf("humidity: %d.%d %%; temperature: %d.%d *C\n", msg[0], msg[1], msg[2], msg[3]);
    fic = fopen("./data.txt", "wt");
    fprintf(fic, "%d.%d;%d.%d\n", msg[0], msg[1], msg[2], msg[3]);
    fclose(fic);
    sleep(2);
  }
  return EXIT_SUCCESS;
}
