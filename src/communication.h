#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

struct sockaddr_in createClient(int port);

int createSocket();

void bindPort(int server, struct sockaddr_in client);

void setHostConnection(char *ip, int port, struct sockaddr_in client);

int sendTo(int server, char *message, struct sockaddr_in destiny);

int receiveFrom(int server, char *message, size_t size, struct sockaddr_in provider);
