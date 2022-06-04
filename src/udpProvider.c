/*
  Compilar - gcc src/udpProvider.c -o udpProvider
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

#define MAX_FILENAME_LENGTH 100
#define MAX_FILE_BUFFER 32
#define CLIENT_PORT 3001
#define PROVIDER_PORT 3002
#define TRACKER_SERVER_PORT 1500

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
    fprintf(
        stderr,
        "%s: cannot open socket \n");
    exit(1);
  }

  /* bind local server port */

  portIsBind = bind( // associa porta ao socket
      server,
      (struct sockaddr *)&providerClient,
      sizeof(providerClient));

  if (portIsBind < 0)
  {
    fprintf(
        stderr,
        "Cannot bind port number %d \n",
        PROVIDER_PORT);
    exit(1);
  }

  fprintf(
      stderr,
      "Waiting for data on port UDP %u\n",
      PROVIDER_PORT);

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
      fprintf(stderr, "Cannot receive data \n");
      continue; // reestarta o loop
    }

    fprintf(stderr, "Received: %s\n", fileName);

    // Abre e lê o arquivo
    char *basePath = "./files/";
    char *filePath = malloc(strlen(basePath) + strlen(fileName) + 1);
    strcpy(filePath, basePath);
    strcat(filePath, fileName);

    FILE *originFile = fopen(filePath, "rb");

    if (originFile == NULL)
    {
      fprintf(stderr, "File transfer failed\n");
      exit(EXIT_FAILURE);
    } // exit(EXIT_FAILURE);

    fseek(originFile, 0, SEEK_END);
    int originFileLength = ftell(originFile);
    fseek(originFile, 0, SEEK_SET);

    char fileBuffer[originFileLength];
    fread(fileBuffer, originFileLength, 1, originFile);

    memset(fileName, 0x0, MAX_FILENAME_LENGTH);
    fclose(originFile);

    // Envia o arquivo
    for (int i = 0; i < originFileLength; i += MAX_FILE_BUFFER)
    {
      char *fileFragment = malloc(MAX_FILE_BUFFER);
      memcpy(fileFragment, fileBuffer + i, MAX_FILE_BUFFER);

      sendto(
          server,
          fileFragment,
          MAX_FILE_BUFFER,
          0,
          (struct sockaddr *)&clientAddr,
          sizeof(clientAddr));

      // Limpa a variavel fileFragment
      fprintf(stderr, "Enviado %d bytes\n", strlen(fileFragment));
      memset(fileFragment, 0x0, MAX_FILE_BUFFER);
    }

    sendto(
        server,
        "END",
        strlen("END") + 1,
        0,
        (struct sockaddr *)&clientAddr,
        sizeof(clientAddr));

    fprintf(stderr, "File sent to client\n");
  } /* end of server infinite loop */

  return 0;
}
