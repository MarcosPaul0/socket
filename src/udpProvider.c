/*
  Compilar - gcc src/udpProvider.c src/utilities.c -o udpProvider -lm
  Executar - ./udpProvider
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
#include <math.h>
#include <time.h>
#include <dirent.h>

#include "utilities.h"

#define MAX_FILENAME_LENGTH 100
#define MAX_FILE_BUFFER 32
#define CLIENT_PORT 3001
#define PROVIDER_PORT 3002
#define TRACKER_SERVER_PORT 1500
#define TRACKER_SERVER_IP "127.0.0.1"

int main()
{
  int server, portIsBind;
  struct sockaddr_in providerClient;

  providerClient.sin_family = AF_INET;
  providerClient.sin_addr.s_addr = htonl(INADDR_ANY);
  providerClient.sin_port = htons(PROVIDER_PORT);

  /* socket creation */
  server = socket(AF_INET, SOCK_DGRAM, 0); // Cria um socket UDP

  if (server < 0)
  {
    printf("cannot open socket \n");
    // printf("%s: cannot open socket \n");
    exit(1);
  }

  /* bind local server port */

  portIsBind = bind( // associa porta ao socket
      server,
      (struct sockaddr *)&providerClient,
      sizeof(providerClient));

  if (portIsBind < 0)
  {
    printf(

        "Cannot bind port number %u\n",
        PROVIDER_PORT);
    exit(1);
  }

  printf(

      "Waiting for data on port UDP %u\n",
      PROVIDER_PORT);

  // paga todos os arquivo do diretorio file e manda para o servidor rastreador
  struct sockaddr_in trackerServerAddr;
  struct hostent *trackerHost;
  // get server IP address (no check if input is IP address or DNS name
  trackerHost = gethostbyname(TRACKER_SERVER_IP);
  if (trackerHost == NULL)
  {
    printf("Nao foi possivel definir o host '%s'\n", TRACKER_SERVER_IP);
    exit(1);
  }

  trackerServerAddr.sin_family = trackerHost->h_addrtype;
  memcpy(
      (char *)&trackerServerAddr.sin_addr.s_addr,
      trackerHost->h_addr_list[0],
      trackerHost->h_length);

  trackerServerAddr.sin_port = htons(TRACKER_SERVER_PORT); 

  /* server infinite loop */
  while (1)
  {
    /* init buffer */
    struct sockaddr_in clientAddr;
    int fileNameWasReceived;

    char fileName[MAX_FILENAME_LENGTH];
    memset(fileName, 0x0, MAX_FILENAME_LENGTH);

    int clientSize = sizeof(clientAddr);
    fileNameWasReceived = recvfrom(
        server,
        fileName,
        MAX_FILENAME_LENGTH,
        0,
        (struct sockaddr *)&clientAddr,
        &clientSize);

    if (fileNameWasReceived < 0)
    {
      printf("Cannot receive data \n");
      continue; // reestarta o loop
    }

    printf("Received: %s\n", fileName);

    // Abre e lê o arquivo
    char basePath[9] = "./files/";
    char *filePath = malloc(strlen(basePath) + strlen(fileName));
    strcpy(filePath, basePath);
    strcat(filePath, fileName);

    FILE *originFile = fopen(filePath, "rb");

    if (originFile == NULL)
    {
      printf("File transfer failed\n");
      exit(EXIT_FAILURE);
    } // exit(EXIT_FAILURE);

    fseek(originFile, 0, SEEK_END);
    int originFileLength = ftell(originFile);
    fseek(originFile, 0, SEEK_SET);

    char fileBuffer[originFileLength];
    fread(fileBuffer, originFileLength, 1, originFile);

    // memset(fileName, 0x0, MAX_FILENAME_LENGTH);
    fclose(originFile);

    memset(fileName, 0x0, MAX_FILENAME_LENGTH);
    memset(filePath, 0x0, strlen(filePath));

    // Envia o tamanho do arquivo
    int fileSize = originFileLength;

    sendto(
        server,
        &fileSize,
        sizeof(fileSize),
        0,
        (struct sockaddr *)&clientAddr,
        sizeof(clientAddr));

    // Número de sequência
    int sequenceNumber = 0;

    int lastDataSize = fileSize % MAX_FILE_BUFFER;

    //  Temporizador
    struct timeval timeVal;

    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeVal, sizeof timeVal);

    // Envia o arquivo
    for (int i = 0; i < originFileLength; i += MAX_FILE_BUFFER)
    {
      int packageSize = MAX_FILE_BUFFER;

      if (i + MAX_FILE_BUFFER > originFileLength)
      {
        packageSize = lastDataSize;
      }

      //  Cria o pacote
      struct package package;

      package.sequenceNumber = sequenceNumber;
      package.dataSize = packageSize;

      //  Copia o conteúdo do arquivo para o pacote
      memset(package.data, 0x0, MAX_FILE_BUFFER);
      memcpy(package.data, fileBuffer + i, packageSize);

      //  Calcula o checksum
      int checkSum = checkSumInHex(package.data, packageSize);
      int checkSumInverse = 255 - checkSum;

      //  Adiciona o checksum inverso ao pacote
      package.checkSum = checkSumInverse;

      //  Envia o pacote
      sendto(
          server,
          &package,
          sizeof(package),
          0,
          (struct sockaddr *)&clientAddr,
          sizeof(clientAddr));

      //  Define o temporizador
      timeVal.tv_sec = 5; //  5 segundos
      timeVal.tv_usec = 0;

      setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeVal, sizeof timeVal);

      struct package responseContent;
      double seconds;

      int response = recvfrom(
          server,
          &responseContent,
          sizeof(responseContent),
          0,
          (struct sockaddr *)&clientAddr,
          &clientSize);

      int sequenceNumberSize = sequenceNumber < 10 ? 1 : floor(log10(sequenceNumber)) + 1;
      char expectedAnswer[4 + sequenceNumberSize];

      memset(expectedAnswer, 0x0, MAX_FILE_BUFFER);
      memcpy(expectedAnswer, "ACK ", 4);
      sprintf(expectedAnswer + 4, "%d", package.sequenceNumber);

      if (response < 0 || strcmp(responseContent.data, expectedAnswer) != 0)
      {
        i -= MAX_FILE_BUFFER; //  Reenvia o pacote perdido
        continue;
      }

      printf("Enviado %d bytes \n\n", package.dataSize);
      sleep(0.2);

      sequenceNumber++;
    }

    struct package package;

    package.sequenceNumber = sequenceNumber;
    package.dataSize = 3;
    package.checkSum = 0;

    memset(package.data, 0x0, MAX_FILE_BUFFER);
    memcpy(package.data, "END", 3);

    sendto(
        server,
        &package,
        sizeof(package),
        0,
        (struct sockaddr *)&clientAddr,
        sizeof(clientAddr));

    // printf("Sending package %d\n", package.sequenceNumber);
    // printf("Package size: %d\n", package.dataSize);
    // printf("Package: %s\n", package.data);

    //  Reseta o temporizador
    timeVal.tv_sec = 0; //  5 segundos
    timeVal.tv_usec = 0;

    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeVal, sizeof timeVal);

    printf("File sent to client\n");
  } /* end of server infinite loop */

  return 0;
}