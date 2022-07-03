/*
  Compilar - gcc src/udpServer.c src/db.c src/communication.c -o udpServer -lpq
  Executar - ./udpServer
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <stdlib.h>

#include "db.h"
#include "communication.h"

#define TRACKER_SERVER_PORT 1500
#define MAX_FILENAME_LENGTH 100
#define CLIENT_PORT 3001
#define PROVIDER_PORT 3002

int main(int argc, char *argv[])
{
  PGconn *pgConnection = connection();

  struct sockaddr_in trackerServer = createClient(TRACKER_SERVER_PORT);

  // socket creation
  int server = createSocket();

  // bind local server port
  bindPort(server, trackerServer);

  fprintf(
      stderr,
      "Waiting for data on port UDP %u\n",
      TRACKER_SERVER_PORT);

  // server infinite loop
  while (1)
  {
    struct sockaddr_in clientAddr; // client address

    // receive filename from client
    char fileName[MAX_FILENAME_LENGTH];
    memset(fileName, 0x0, MAX_FILENAME_LENGTH);

    int clientAddrLen = sizeof(clientAddr);
    int fileNameWasReceived = recvfrom(
        server,
        fileName,
        MAX_FILENAME_LENGTH,
        0,
        (struct sockaddr *)&clientAddr,
        &clientAddrLen);

    printf("Received file name: %s\n", fileName);

    if (fileNameWasReceived < 0)
    {
      fprintf(stderr, "Cannot receive data \n");
      continue; // reestarta o loop
    }

    // Se o usuário não existe, insere no banco
    char *receiverUserIp = findUserByIp(pgConnection, inet_ntoa(clientAddr.sin_addr));

    if (receiverUserIp == NULL)
    {
      insertNewUser(pgConnection, inet_ntoa(clientAddr.sin_addr));
    }

    // Procura o usuário que possui o arquivo informado
    char *providerUserAddress = findReceiverUserIpByFileName(pgConnection, fileName);

    printf("Provider user address: %s\n", providerUserAddress);

    if (providerUserAddress == NULL)
    {
      fprintf(stderr, "File does not exists in our database\n");
      sendto(server, "404", strlen("404"), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
      continue;
    }

    // Retorna o endereço de ip do usuário que possui o arquivo
    sendTo(server, providerUserAddress, clientAddr);

    // if (isSended < 0)
    // {
    //   fprintf(stderr, "Cannot send data \n");
    //   continue; // reestarta o loop
    // }
    // // Caso a tranferência seja realizada com sucesso, registra que o usuário também possui o arquivo
    // insertNewFileToUser(pgConnection, receiverUserIp, fileName);
  } /* end of server infinite loop */

  return 0;
}
