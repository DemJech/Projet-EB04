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

void send_file_data(int sockfd, struct sockaddr_in addr, char * msg) {
  int n;
  // Sending the data
  n = sendto(sockfd, msg, MSG_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
  if (n == -1) {
    perror("Could not send data to the server.");
    exit(1);
  }
  bzero(msg, MSG_SIZE);

  // Envoie de 'END' pour signifier la fin du message
  strcpy(msg, "END");
  sendto(sockfd, msg, MSG_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
}

int main(int argv, int **argc) {

  // Définition de l'IP et du port
  char *ip = "192.168.43.201";
  const int port = 8080;

  // Déclaration du socket
  int server_sockfd;
  struct sockaddr_in server_addr;

  int fd, ret;
  char msg[MSG_SIZE+1] = {0,0,0,0,0};

  fd = open("/dev/rtdm/rtdm_DHT11_0", O_RDONLY); //Ouverture du driver
  if (fd < 0) {
    printf("Open error %d\n", fd);
    exit(fd);
  }

  // Création du socket UDP
  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_sockfd < 0) {
    perror("[ERROR] socket error");
    exit(1);
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  while(TRUE) {
    ret = read(fd, msg, MSG_SIZE);  //Lecture du driver
    if (ret <= 0) {
      printf("%d\n", msg[0]);
      printf("Read error %d\n", ret);
      exit(ret);
    }
    printf("humidity: %d.%d %%; temperature: %d.%d *C\n", msg[0], msg[1], msg[2], msg[3]);

    fprintf(fic, "%d.%d;%d.%d\n", msg[0], msg[1], msg[2], msg[3]); //Ecriture des données
    send_file_data(server_sockfd, server_addr, msg); //Envoie des données
    write(fd, &msg, MSG_SIZE+1);  //Sans l'écriture, le programme bug, à la prochaine relecture
    sleep(2); //Attente d'une nouvelle mesure
  }
  return EXIT_SUCCESS;
}
