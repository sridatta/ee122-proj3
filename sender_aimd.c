#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "packet.h"
#include "sender.h"
#include "queue.h"

const int NUM_PACKETS = 500;
const int MAX_WINDOW = 128;
const int SEQ_MAX = 129;

int main(int argc, char *argv[]){

  if(argc != 6){
    printf("usage: host port R stream_id window_size\n");
    exit(0);
  }

  char *endptr;
  long R = strtol(argv[3], &endptr, 10);
  char *stream_id = argv[4];
  int window_size = strtol(argv[5], NULL, 10);

  if(endptr != (argv[3] + strlen(argv[3]))) {
    fprintf(stderr, "R must be an integer (milliseconds).\n");
    exit(1);
  }


	struct addrinfo* p;
  int send_sock = send_port(argv[1], argv[2], &p);
  int recv_sock = recv_port(NULL, argv[2]);


  printf("Stream id = %s\n", stream_id);
  ee122_packet pkt;
  pkt.R = R;
  pkt.stream = *stream_id;
  printf("pkt.stream = %c\n", pkt.stream);
  pkt.avg_len = 0;
  pkt.window_size = window_size;

  ee122_packet rcv_pkt;

  struct timespec sleep_spec;
  sleep_spec.tv_sec = 0;

  struct timeval curr_time, start_time, last_generated, diff_time;
  gettimeofday(&start_time, NULL);
  gettimeofday(&last_generated, NULL);

  int seq_no = 0;
  int in_flight = 0; 

  struct timeval timeouts[SEQ_MAX];
  ee122_packet packets[SEQ_MAX];

  int i;
  struct timeval zero_timeout;
  zero_timeout.tv_usec = 0;
  zero_timeout.tv_sec = 0;

  for(i = 0; i < SEQ_MAX; i++){
    memcpy(&timeouts[i], &zero_timeout, sizeof(zero_timeout));
  }

  bytequeue q;
  bytequeue_init(&q, sizeof(ee122_packet), window_size);

  float rtt = 1000*1000;
  char buff[sizeof(ee122_packet)];

  float next_wait = rand_poisson(R);
  printf("next wait: %f\n", next_wait);

  int bytes_read;
  struct sockaddr src_addr;
  int src_len = sizeof(src_addr);
  unsigned int down = 1;
  unsigned char ack_expected = 0;
  unsigned int garbage = 0;
  unsigned int padding = 56;
  unsigned total_attempts = 0;
  unsigned seconds = 0;
  unsigned errors = 0;
  unsigned acks = 0;
  int done = 0;

  printf("Starting\n");
  while(1){
    gettimeofday(&curr_time, NULL);

    timeval_subtract(&diff_time, &curr_time, &start_time);

    if (diff_time.tv_sec * 1000000 + diff_time.tv_usec > seconds * 1000000) {
      printf("%f,%d,%d\n", rtt, window_size, seconds);
      seconds++;
    }
    // Stop sending after 60 seconds
    if(diff_time.tv_sec * 1000000 + diff_time.tv_usec > 60*1000000){ done = 1; }

    if (done && in_flight == 0) { break; }
    // Check timeouts. If timeout reached, retransmit that packet and all following.
    int retransmitting = 0;
    int i;
    struct timeval timeout = timeouts[ack_expected];

    timeval_subtract(&diff_time, &curr_time, &timeout);
    if(diff_time.tv_sec * 1000000 + diff_time.tv_usec > rtt){
      retransmitting = 1;
    } 
    
    if(retransmitting){
      i = ack_expected;
      do {
        timeout = timeouts[i];
        if(timeout.tv_sec == 0 && timeout.tv_usec == 0) { // TODO will this ever run?
          i = (i + 1) % (SEQ_MAX);
          timeout = timeouts[ack_expected];
          continue;
        }
        // Reset the timeout for this packet and send.
        gettimeofday(&(timeouts[i]), NULL);
        total_attempts++;
        errors++;
        packets[i].total_attempts = total_attempts;
        serialize_packet(buff, packets[i]);
        sendto(send_sock, buff, sizeof(packets[i]), 0, p->ai_addr, p->ai_addrlen);
        i = (i + 1) % SEQ_MAX;
      } while (i != (ack_expected+window_size)%SEQ_MAX);
      window_size = window_size / 2;
      if (window_size == 0) window_size = 1;
    }

    // Check if new packet should be generated by now
    timeval_subtract(&diff_time, &curr_time, &last_generated);
    if(diff_time.tv_sec * 1000000 + diff_time.tv_usec > next_wait * 1000){
      // Enqueue the packet.
      bytequeue_push(&q, &pkt);
      next_wait = rand_poisson(R);
    }

    // Read from socket. Increase available window for each ack received
    int bytes_read = recvfrom(recv_sock, buff, sizeof(ee122_packet), 0, &src_addr, &src_len);
    if(bytes_read > 0){
      rcv_pkt = deserialize_packet(buff);
      if(rcv_pkt.stream == 'Z' && rcv_pkt.seq_number == ack_expected){
        struct timeval timeout_start = rcv_pkt.timestamp;

        // Learn RTT
        timeval_subtract(&diff_time, &curr_time, &timeout_start);
        //printf("RTT difftime, tv_sec == %d, tv_usec == %d\n", diff_time.tv_sec, diff_time.tv_usec); 
        rtt = (0.6 * (diff_time.tv_sec * 1000000 + diff_time.tv_usec)) + 0.4*rtt;

        // Reset this timeout
        timeouts[rcv_pkt.seq_number].tv_usec = 0;
        timeouts[rcv_pkt.seq_number].tv_sec = 0;

        in_flight--;

        ack_expected = (ack_expected+1) % (SEQ_MAX);
        acks++;
        if (acks > window_size) {
          window_size++;
          if (window_size >= MAX_WINDOW)
            window_size = MAX_WINDOW;
          acks = 0;
        }
        //if (available_window > window_size) available_window = window_size;
      }
      else {
        //printf("Received ACK with seq = %d, expecting = %d\n", rcv_pkt.seq_number, (last_received + 1) % window_size);
      }
    }

    // Check available window. If available, dequeue and send a packet.
    if(in_flight <= window_size && q.filled != 0 && !done){
      //printf("window size == %d, available window == %d\n", window_size, available_window);
      bytequeue_pop(&q, &pkt);

      total_attempts++;

      pkt.seq_number = (seq_no) % (SEQ_MAX);
      pkt.timestamp = curr_time;
      pkt.total_attempts = total_attempts;
      pkt.timeout = rtt;
      
      serialize_packet(buff, pkt);

      //Store the packet into the packets buffer for possible transmission
      memcpy(&packets[pkt.seq_number], &pkt, sizeof(pkt));

      sendto(send_sock, buff, sizeof(pkt), 0, p->ai_addr, p->ai_addrlen);
      pkt = deserialize_packet(buff);
      in_flight += 1;

      // Set this timeout
      gettimeofday(&timeouts[pkt.seq_number], NULL);

      seq_no++;
    }

  }

  printf("Total errors: %d. Total successes: %d. Total total:%d\n", errors, total_attempts - errors, total_attempts);

  close(send_sock);
  close(recv_sock);
  exit(0);
}

int recv_port(char* host, char* port) {
  int status;
  int sockfd;

  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *res, *p;

  if ((status = getaddrinfo(host, port, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  for(p = res; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        perror("Error creating router listener socket");
        continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("Error binding router listener socket");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "router: failed to bind socket\n");
    return -2;
  }

	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

  return sockfd;
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
        continue;
    }

    break;
  }

  if(p == NULL){
    fprintf(stderr, "No valid addresses");
    exit(-2);
  }
  printf("p = %d, ", p);

	*pptr = p;
  return sockfd;
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


float rand_poisson(long interpacket)
{
  float rateParameter = 1.0/((float) interpacket);
  return -logf(1.0f - (float) random() / (RAND_MAX)) / rateParameter;
}
