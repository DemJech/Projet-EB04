#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 1024
#define TRUE 1

void write_file(int sockfd, struct sockaddr_in addr) {
  FILE *fp;
  char *filename = "server.txt";
  int n;
  char buffer[SIZE];
  socklen_t addr_size;

  fp = fopen(filename, "at");

  while(1) {
    addr_size = sizeof(addr);
    n = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr*) &addr, &addr_size);

    if (strcmp(buffer, "END") == 0) {
      printf("%s\t%d", buffer, strcmp(buffer, "END"));
      break;
      return;
    }

    printf("[RECEIVING] Data: %s", buffer);
    fprintf(fp, "%s", buffer);
    bzero(buffer, SIZE);
  }

  fclose(fp);
  return;
}

int main(int argc, char ** argv) {
  char *ip = "192.168.43.201";                       //Addr IP de rebouclage
  int port = 8080;

  int server_sockfd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[SIZE];

  int e;

  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_sockfd < 0) {
    printf("[ERROR] Socket error, exit code: %d\n", server_sockfd);
    exit(server_sockfd);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  e = bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (e < 0) {
    printf("[ERROR] Bind error, exit code : %d\n", e);
    exit(e);
  }

  printf("[STARTING] UDP File Server started.\n");
  while (TRUE) {
    write_file(server_sockfd, client_addr);
    sleep(2);
  }

  printf("[SUCCESS] Data transfer complete.\n");
  printf("[CLOSING] Closing the server.\n");
  close(server_sockfd);
  return 0;
}
