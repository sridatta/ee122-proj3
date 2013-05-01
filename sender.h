float rand_poisson(long lambda);
int main(int argc, char *argv[]);
int recv_port(char* host, char* port);
int send_port(char* host, char* port, struct addrinfo** pptr);
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y);
