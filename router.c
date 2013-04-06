#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include "router.h"
#include "queue.h"
#include "packet.h"

const unsigned long MAX_PACKETS = 500;

int main(int argc, char* argv[]) {
  int streams = 1;

  /* Arguments: L, B, duty cycle, destination port 1, destination port 2 (optional) */
  if (argc != 5 && argc != 8 && argc != 9) {
    fprintf(stderr, "Usage: %s L B source_port_1 destination_port_1 [source_port_2 destination_port_2 duty_cycle [prioritized]]\n", argv[0]);
    return -1;
  }
  if (argc > 6) {
    streams = 2;
  }
	int prioritized = 0;
	if (argc == 9)
		prioritized = 1;

	struct addrinfo* p_1;
  int insock_1 = recv_port(NULL, argv[3]);
  int outsock_1 = send_port("localhost", argv[4], &p_1);

	struct addrinfo* p_2 = NULL;
  int insock_2 = -1;
  int outsock_2 = -1;
  int duty_cycle = 100;
  if (streams == 2) {
    insock_2 = recv_port(NULL, argv[5]);
    outsock_2 = send_port("localhost", argv[6], &p_2);
    duty_cycle = strtol(argv[7], NULL, 10);
    if (duty_cycle == 0) {
      printf("Duty cycle zero or not numeric. Create a single-queue implementation by omitting later arguments.");
      return -3;
    }
  }

  int L = strtol(argv[1], NULL, 10);
  if (L <= 0) {
    printf("L negative, zero or not numeric.");
    return -3;
  }

  int B = strtol(argv[1], NULL, 10);
  if (B <= 0) {
    printf("B negative, zero or not numeric.");
    return -3;
  }

	unsigned capacity_1 = (unsigned)(B * (((double)(duty_cycle)) / 100.0));
	unsigned capacity_2 = B - capacity_1;
	bytequeue queue_1, queue_2;
	unsigned long sum_1 = 0, sum_2 = 0;
	if (bytequeue_init(&queue_1, sizeof(ee122_packet), capacity_1) < 0) {
		perror("Memory error allocating queue 1");
	}
	if (streams == 2 && bytequeue_init(&queue_2, sizeof(ee122_packet), capacity_2) < 0) {
		perror("Memory error allocating queue 2");
	}
	float avg_1, avg_2;

  unsigned long count = 0;
	unsigned long sent_count = 0;
	struct timeval last_time, curr_time, diff_time;
	int which_queue = 1;
	if (gettimeofday(&last_time, NULL) < 0)
		perror("gettimeofday error");
  while (count < MAX_PACKETS) {
    int read_count = 0;
    ee122_packet p;
    struct sockaddr src_addr;
    int src_len = sizeof(src_addr);
    read_count = recvfrom(insock_1, &p, sizeof(p), 0, &src_addr, &src_len);
		/* read_count == -1 and errno == EWOULDBLOCk or EAGAIN indicates nothing there */
		if (read_count == -1 && errno != EWOULDBLOCK && errno != EAGAIN)
			perror("Error reading from socket 1");
		else if (read_count == 0)
			perror("Socket 1 closed");
		else if (read_count > 0) {
			/* push packet */
			if (p.stream == 'A') {
				bytequeue_push(&queue_1, &p);
			}
			if (streams == 2) {
				if (p.stream == 'B') {
					bytequeue_push(&queue_2, &p);
				}
			}
			count++;
		}
		
		gettimeofday(&curr_time, NULL);
		timeval_subtract(&diff_time, &curr_time, &last_time);
      
		if (diff_time.tv_sec * 1000000 + diff_time.tv_usec > L * 1000) {
			/* update average queue length */
			sent_count++;
			sum_1 += queue_1.filled;
			avg_1 = ((float)(sum_1)) / ((float)(sent_count));
			if (streams == 2) {
				sum_2 += queue_2.filled;
				avg_2 = ((float)(sum_2)) / ((float)(sent_count));
			}

			ee122_packet s;
			int write_count = 0;
			if (prioritized) {
				if (queue_1.filled) {
					if (bytequeue_pop(&queue_1, &s) == 0) {
						s.avg_len = avg_1;
						sendto(outsock_1, &s, sizeof(s), 0, p_1->ai_addr, p_1->ai_addrlen);
					}
				}
				else {
					if (bytequeue_pop(&queue_2, &s) == 0) {
						s.avg_len = avg_2;
						sendto(outsock_2, &s, sizeof(s), 0, p_2->ai_addr, p_2->ai_addrlen);
					}
				}
			}
			else {
				if (streams == 2) {
  				if (which_queue == 1) {
  					if (bytequeue_pop(&queue_1, &s) == 0) {
						  s.avg_len = avg_1;
							sendto(outsock_1, &s, sizeof(s), 0, p_1->ai_addr, p_1->ai_addrlen);
  					}
  					which_queue = 2;
  				}
  				else {
  					if (bytequeue_pop(&queue_2, &s) == 0) {
							s.avg_len = avg_2;
							sendto(outsock_2, &s, sizeof(s), 0, p_2->ai_addr, p_2->ai_addrlen);
  					}
  					which_queue = 1;
					}
				}
				else {
          if (bytequeue_pop(&queue_1, &s) == 0) {
            s.avg_len = avg_1;
            sendto(outsock_1, &s, sizeof(s), 0, p_1->ai_addr, p_1->ai_addrlen);
          }
				}
			}

			/* update last_time */
			last_time = curr_time;

		}
	}
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
        perror("Error creating listener socket");
        continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("Error binding listener socket");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "listener: failed to bind socket\n");
    return -2;
  }

	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

  return sockfd;
}

int send_port(char* host, char* port, struct addrinfo** pptr) {
  int status;
  int sockfd;

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

