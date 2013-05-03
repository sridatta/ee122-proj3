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
#include "receiver.h"

const int NUM_PACKETS = 500;

int main(int argc, char *argv[]){

  if(argc != 4){
    printf("usage: port sender_hostname ack_port\n");
    exit(0);
  }

  srand(time(0)); // init random

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

	struct addrinfo* send_p;
  int outsock = send_port(argv[2], argv[3], &send_p);

  struct timeval timeout;

  timeout.tv_sec = 4;
  timeout.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&timeout, sizeof(struct timeval));

  struct sockaddr src_addr;
  int src_len = sizeof src_addr;
  ee122_packet pkt;

  int num_rcv = 0;
  int bytes_read = 0;
  unsigned long attempted = 0;
  float avg_len;

  unsigned char buff[128];
  long R;

  int delay;
  struct timespec sleep_spec;
  sleep_spec.tv_sec = 0;

  struct timeval last_time, curr_time, diff_time;
  double sum = 0.0;

  int seq_expected = 0;
  while(bytes_read = recvfrom(sockfd, buff, sizeof(ee122_packet), 0, &src_addr, &src_len)){
    pkt = deserialize_packet(buff);
    if(bytes_read == -1){
      if(num_rcv > 0){
        break;
      }
    } else {

      //printf("Received packet with seq no: %d\n", ntohl(*((uint32_t*) buff)));
      if(num_rcv == 0) {
        R = pkt.R;
      } else {
      }

      if (flip_state) {
        delay = rand() % 15;
      }
      else {
        delay = rand() % 5;
      }

      sleep_spec.tv_nsec = delay * 1000000;
      nanosleep(&sleep_spec, &sleep_spec);

      if(pkt.seq_number == seq_expected){
        seq_expected = (seq_expected + 1) % (pkt.window_size + 1);

        // Do the calcs
        num_rcv++;
        attempted = pkt.total_attempts;
        avg_len = pkt.avg_len;
        gettimeofday(&curr_time, NULL);
        last_time = pkt.timestamp;
        timeval_subtract(&diff_time, &curr_time, &last_time);
        sum += ((double)(diff_time.tv_sec)) + (diff_time.tv_usec / 1000000.0);
      }

      // Send ACK
      ee122_packet ack;
      ack = pkt;
      ack.stream = 'Z';

      unsigned char buff[sizeof(ack)];
      serialize_packet(buff, ack);
			sendto(outsock, buff, sizeof(ack), 0, send_p->ai_addr, send_p->ai_addrlen);
    }
  }

  printf("%d,%f,%lf\n", pkt.R, ((float) num_rcv)/attempted, avg_len);
  printf("num received: %lu, attempted: %d\n", num_rcv, attempted);

  freeaddrinfo(res);
  close(sockfd);
  exit(0);
}

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) { 
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
                                                                   }
                                  
  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  return x->tv_sec < y->tv_sec;
}

int flip_state () {
    const int seconds = 5;
    static struct timeval lastflip;
    static int first = 1;
    static int ret = 0;
    if (first) {
        gettimeofday(&lastflip, NULL);
    }

    struct timeval result, currtime;
    gettimeofday(&currtime, NULL);
    timeval_subtract(&result, &currtime, &lastflip);
    if (result.tv_sec >= seconds) {
        if (ret) ret = 0;
        else ret = 1;
        lastflip = currtime;
    }
    return ret;
}

int send_port(char* host, char* port, struct addrinfo** pptr) {
  int status;
  int sockfd;

  printf("Generating send socket\n");
  printf("Host = %s, port = %s\n", host, port);

  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *res, *p;

  if ((status = getaddrinfo(host, port, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  for(p = res; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        fprintf(stderr, "socket() error: %s\n", strerror(errno));
        continue;
    }

    break;
  }

  if(p == NULL){
    fprintf(stderr, "No valid addresses");
    exit(-2);
  }

	*pptr = p;
  return sockfd;
}
