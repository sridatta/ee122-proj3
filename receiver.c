#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "packet.h"

const int NUM_PACKETS = 500;

int main(int argc, char *argv[]){

  if(argc != 2){
    printf("usage: port\n");
    exit(0);
  }

  int status;
  int sockfd;

  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *res, *p;

  if ((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  for(p = res; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        perror("listener: socket");
        continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("listener: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "listener: failed to bind socket\n");
    return 2;
  }


  struct sockaddr src_addr;
  int src_len = sizeof src_addr;
  ee122_packet pkt;

  int num_rcv = 0;
  unsigned long num_expected;

  while(recvfrom(sockfd, &pkt, sizeof(ee122_packet), 0, &src_addr, &src_len)){
    num_rcv++;
    num_expected = pkt.num_expected;
    printf("Received: %d, Expected: %lu \n", num_rcv, num_expected);
  }


  freeaddrinfo(res);
  close(sockfd);
  exit(0);
}
