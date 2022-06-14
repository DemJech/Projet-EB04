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

  fp = fopen(filename, "at"); //Ouverture du fichier
  if (fp < 0) {
    printf("Open error %d\n", fp);
    exit(fp);
  }

  while(1) {//Réception des données tant que le "END" n'a pas été reçu
    addr_size = sizeof(addr);
    recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr*) &addr, &addr_size);

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
}

int main(int argc, char ** argv) {
  int e;

  //Définition de l'IP et du port
  char *ip = "192.168.43.201";
  int port = 8080;

  //Création du socket
  int server_sockfd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[SIZE];
  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_sockfd < 0) {
    printf("[ERROR] Socket error, exit code: %d\n", server_sockfd);
    exit(server_sockfd);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip);

  //Démarrage du serveur
  e = bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (e < 0) {
    printf("[ERROR] Bind error, exit code : %d\n", e);
    exit(e);
  }

  printf("[STARTING] UDP File Server started.\n");
  while (TRUE) {
    write_file(server_sockfd, client_addr); //Ecriture des données
    sleep(2);
  }

  printf("[CLOSING] Closing the server.\n");
  close(server_sockfd);
  return 0;
}
