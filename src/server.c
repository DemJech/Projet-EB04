#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 1024

int main(int argc, char ** argv) {
  char *ip = "127.0.0.1";
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

  e = bind(server_sockfd, (struct sockaddr_in *)&server_addr, sizeof(server_addr));
  if (e < 0) {
    printf("[ERROR] Bind error, exit code : %d\n", e);
    exit(e);
  }

  printf("[STARTING] UDP File Server started.\n", );
}
