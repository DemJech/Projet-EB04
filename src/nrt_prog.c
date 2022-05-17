#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

struct Measures {
  int temperature = 0;
  int humidity = 0;
}

int main(int argv, int **argc) {
  int fd, ret;
  Measures measures;
  fd = open("/dev/rtdm/rtdm_DHT11_0", O_RDWR);
  if (fd < 0) {
    printf("Open error %d\n", fd);
    exit(fd);
  }
  while(true) {
    ret = read(fd, measures, sizeof(measures));
    if (ret <= 0) {
      printf("Read error %d\n", ret);
      exit(ret);
    }
  }
}
