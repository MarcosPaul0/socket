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
  int messageIsSended;

  struct sockaddr_in trackerServer = createClient(TRACKER_SERVER_PORT);

  // socket creation
  int server = createSocket();

  // bind local server port
  bindPort(server, trackerServer);

  printf(
      "Esperando pelo dado na porta %u\n",
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

    printf("Cliente buscando o arquivo: %s\n", fileName);

    if (fileNameWasReceived < 0)
    {
      printf("Nao foi possivel receber o nome do arquivo\n");
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

    printf("Endereco do provedor: %s\n", providerUserAddress);

    if (providerUserAddress == NULL)
    {
      printf("Arquivo nao existe no banco de dados\n");
      sendto(server, "404", strlen("404"), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
      continue;
    }

    // Retorna o endereço de ip do usuário que possui o arquivo
    messageIsSended = sendTo(server, providerUserAddress, clientAddr);

    if (messageIsSended < 0)
    {
      printf("Nao foi possivel enviar o endereco do provedor\n");
      continue; // reestarta o loop
    }

    // Espera a menssagem do cliente que o arquivo foi recebibo
    memset(fileName, 0x0, MAX_FILENAME_LENGTH);
    messageIsSended = recvfrom(
        server,
        fileName,
        MAX_FILENAME_LENGTH,
        0,
        (struct sockaddr *)&clientAddr,
        &clientAddrLen);

    if (messageIsSended < 0)
    {
      printf("Nao foi possivel receber a menssagem de arquivo recebido do cliente\n");
      continue; // restarta o loop
    }

    printf("Cliente recebeu o arquivo: %s\n", fileName);

    // Cadastra o arquivo no banco de dados
    // Verifica se o cliente já possui um arquivo com o mesmo nome
    char *fileId = findFileByUserIp(pgConnection, fileName, inet_ntoa(clientAddr.sin_addr));

    if (fileId == NULL)
    {
      insertNewFileToUser(pgConnection, inet_ntoa(clientAddr.sin_addr), fileName);
    }
  } /* end of server infinite loop */

  return 0;
}
