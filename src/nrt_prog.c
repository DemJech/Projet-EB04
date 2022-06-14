#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 1024
#define MSG_SIZE 4
#define TRUE 1

void send_file_data(FILE* fp, int sockfd, struct sockaddr_in addr, char * msg) {
  int n;
  // Sending the data
  printf("[SENDING] Data: %d.%d;%d.%d\n", msg[0], msg[1], msg[2], msg[3]);

  n = sendto(sockfd, msg, MSG_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
  if (n == -1)
  {
    perror("[ERROR] sending data to the server.");
    exit(1);
  }
  bzero(msg, MSG_SIZE);

  // Sending the 'END'
  strcpy(msg, "END");
  sendto(sockfd, msg, MSG_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
  fclose(fp);
}

int main(int argv, int **argc) {

  // Defining the IP and Port
  char *ip = "192.168.43.201";
  const int port = 8080;

  // Defining variables
  int server_sockfd;
  struct sockaddr_in server_addr;

  int fd, ret;
  char msg[MSG_SIZE+1] = {0,0,0,0,0};
  FILE * fic;

  fic = fopen("./data.txt", "rwt");
  if (fic == NULL) {
    printf("Open error %d\n", 1);
    exit(1);
  }

  fd = open("/dev/rtdm/rtdm_DHT11_0", O_RDONLY);
  if (fd < 0) {
    printf("Open error %d\n", fd);
    exit(fd);
  }

  // Creating a UDP socket
  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_sockfd < 0)
  {
    perror("[ERROR] socket error");
    exit(1);
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  while(TRUE) {
    ret = read(fd, msg, sizeof(msg));
    if (ret <= 0) {
      printf("%d\n", msg[0]);
      printf("Read error %d\n", ret);
      exit(ret);
    }
    printf("humidity: %d.%d %%; temperature: %d.%d *C\n", msg[0], msg[1], msg[2], msg[3]);
    fic = fopen("./data.txt", "rwt");
    if (fic == NULL) {
      printf("Open error %d\n", 1);
      exit(1);
    }
    fprintf(fic, "%d.%d;%d.%d\n", msg[0], msg[1], msg[2], msg[3]);
    send_file_data(fic, server_sockfd, server_addr, msg);
    write(fd, msg, MSG_SIZE+1);
    sleep(2);
  }
  return EXIT_SUCCESS;
}
