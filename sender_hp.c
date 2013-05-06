#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "packet.h"
#include "sender.h"

const int NUM_PACKETS = 500;

int main(int argc, char *argv[]){

  if(argc != 6){
    printf("usage: host port R stream_id filename\n");
    exit(0);
  }

  char *endptr;
  long R = strtol(argv[3], &endptr, 10);
  char *stream_id = argv[4];

  if(endptr != (argv[3] + strlen(argv[3]))) {
    fprintf(stderr, "R must be an integer (milliseconds).\n");
    exit(1);
  }

  int status;
  int sockfd;

  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *res, *p;

  if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  for(p = res; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        fprintf(stderr, "socket() error");
        continue;
    }

    break;
  }

  if(p == NULL){
    fprintf(stderr, "No valid addresses");
    exit(2);
  }


  ee122_packet pkt;
  pkt.R = R;
  pkt.stream = *stream_id;
  pkt.avg_len = 0;

  int read_count;
  char fbuff[sizeof(pkt.payload)];
  memset(fbuff,0,sizeof(fbuff));
  FILE *fd = fopen(argv[5], "r");
  if(NULL == fd) {
    fprintf(stderr, "fopen() error\n");
    return 1;
  }

  struct timespec sleep_spec;
  sleep_spec.tv_sec = 0;

  struct timeval curr_time;

  struct timeval start_time, diff_time;
  gettimeofday(&start_time, NULL);

  int seq_no;
  while(1) {
    gettimeofday(&curr_time, NULL);
    timeval_subtract(&diff_time, &curr_time, &start_time);
    if(diff_time.tv_sec * 1000000 + diff_time.tv_usec > 60*1000000){ break; }
    
    sleep_spec.tv_nsec = rand_poisson(R)*1000000;
    pkt.seq_number = seq_no;
    pkt.timestamp = curr_time;
    read_count = fread(pkt.payload, sizeof(char), sizeof(pkt.payload), fd);
    
    if (feof(fd)) break;
    if(ferror(fd)){
       fprintf(stderr, "error: %s\n", strerror(errno));
       exit(3);
    }

    sendto(sockfd, &pkt, sizeof(ee122_packet), 0, p->ai_addr, p->ai_addrlen);
    nanosleep(&sleep_spec, &sleep_spec);
  }

  freeaddrinfo(res);
  close(sockfd);
  exit(0);
}

float rand_poisson(long interpacket)
{
  float rateParameter = 1.0/((float) interpacket);
  return -logf(1.0f - (float) random() / (RAND_MAX)) / rateParameter;
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
