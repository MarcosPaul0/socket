#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <stdlib.h>

#include "communication.h"

struct sockaddr_in createClient(int port)
{
  struct sockaddr_in client;
  client.sin_family = AF_INET;
  client.sin_addr.s_addr = htonl(INADDR_ANY);
  client.sin_port = htons(port);

  return client;
}

int createSocket()
{
  int server = socket(AF_INET, SOCK_DGRAM, 0); // Cria um socket UDP

  if (server < 0)
  {
    fprintf(stderr, "Cannot open socket\n");
    exit(1);
  }

  return server;
}

void bindPort(int server, struct sockaddr_in client)
{
  int portIsBind = bind( // associa porta ao socket
      server,
      (struct sockaddr *)&client,
      sizeof(client));

  if (portIsBind < 0)
  {
    fprintf(stderr, "Cannot bind port\n");
    exit(1);
  }
}

void setHostConnection(char *ip, int port, struct sockaddr_in client) {
  struct hostent *host;
  // get server IP address (no check if input is IP address or DNS name
  host = gethostbyname(ip);
  if (host == NULL)
  {
    fprintf(stderr, "Unknown host '%s'\n", ip);
    exit(1);
  }

  client.sin_family = host->h_addrtype;
  memcpy(
      (char *)&client.sin_addr.s_addr,
      host->h_addr_list[0],
      host->h_length);

  client.sin_port = htons(port);
}

int sendTo(int server, char *message, struct sockaddr_in destiny)
{
  int isSended = sendto(
      server,
      message,
      strlen(message) + 1,
      0,
      (struct sockaddr *)&destiny,
      sizeof(destiny));

  return isSended;
}

int receiveFrom(int server, char *message, size_t size, struct sockaddr_in provider)
{
  int providerSize = sizeof(provider);
  int isSended = recvfrom(
      server,
      message,
      size + 1,
      0,
      (struct sockaddr *)&provider,
      &providerSize);

  return isSended;
}