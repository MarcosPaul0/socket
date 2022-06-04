/*
  Compilar - gcc src/udpClient.c -o udpClient
  Executar - ./udpClient
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>   /* memset() */
#include <sys/time.h> /* select() */
#include <stdlib.h>

#define MAX_FILENAME_LENGTH 100
#define MAX_PROVIDER_IP_LENGTH 16
#define MAX_FILE_BUFFER 32
#define CLIENT_PORT 3001
#define PROVIDER_PORT 3002
#define TRACKER_SERVER_PORT 1500
#define TRACKER_SERVER_IP "127.0.0.1"

/*
  argc = número de argumentos passsados na linha de comando
  argv[0] = ./nome_do_programa
  argv[1] = ip do servidor
  argv[2] = mensagem
*/
int main()
{
  int server, portIsBind;
  struct sockaddr_in clientAddr;

  clientAddr.sin_family = AF_INET;                // Família de protocolo - IPv4
  clientAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Edereço local utilizado pelo socket convertido pelo htonl
  clientAddr.sin_port = htons(CLIENT_PORT);       // Porta local utilizada pelo socket convertida pelo htons

  server = socket(AF_INET, SOCK_DGRAM, 0); // Cria um socket UDP
  if (server < 0)
  {
    fprintf(stderr, "Cannot open socket\n");
    exit(1);
  }

  // Bind a port number to the socket
  portIsBind = bind(
      server,
      (struct sockaddr *)&clientAddr,
      sizeof(clientAddr));

  if (portIsBind < 0)
  {
    fprintf(stderr, "Cannot bind port\n");
    exit(1);
  }

  struct sockaddr_in trackerServerAddr;
  struct hostent *trackerHost;
  // get server IP address (no check if input is IP address or DNS name
  trackerHost = gethostbyname(TRACKER_SERVER_IP);
  if (trackerHost == NULL)
  {
    fprintf(stderr, "Unknown host '%s'\n", TRACKER_SERVER_IP);
    exit(1);
  }

  trackerServerAddr.sin_family = trackerHost->h_addrtype;
  memcpy(
      (char *)&trackerServerAddr.sin_addr.s_addr,
      trackerHost->h_addr_list[0],
      trackerHost->h_length);

  trackerServerAddr.sin_port = htons(TRACKER_SERVER_PORT);

  while (1)
  {
    // O usuário insere o arquivo desejado
    fprintf(stderr, "Digite o nome do arquivo: ");
    char fileName[100];
    scanf("%s", fileName);

    // Envia o nome do arquivo requerido
    int messageIsSended = sendto(
        server,
        fileName,
        strlen(fileName) + 1,
        0,
        (struct sockaddr *)&trackerServerAddr,
        sizeof(trackerServerAddr));

    if (messageIsSended < 0)
    {
      fprintf(stderr, "Cannot send data\n");
      close(server);
      exit(1);
    }

    // Recebe o ip do client que possui o arquivo
    char providerIp[MAX_PROVIDER_IP_LENGTH];
    int providerIpWasReceived;

    memset(providerIp, 0x0, MAX_PROVIDER_IP_LENGTH);

    int trackerSize = sizeof(trackerServerAddr);
    providerIpWasReceived = recvfrom(
        server,
        providerIp,
        MAX_PROVIDER_IP_LENGTH,
        0,
        (struct sockaddr *)&trackerServerAddr,
        &trackerSize);

    if (providerIpWasReceived < 0)
    {
      fprintf(stderr, "Cannot receive data\n");
    }

    if (strcmp(providerIp, "404") == 0)
    {
      fprintf(stderr, "File does not exists in our database\n");
      close(server);
      continue;
    }

    fprintf(stderr, "\n\nO arquivo %s está disponível no cliente %s\n", fileName, providerIp);
    fprintf(stderr, "Fazendo conexão com o cliente %s ...\n\n", providerIp);

    // Solicita o envio do arquivo para o usuário provedor
    // Envia o nome do arquivo requerido
    struct sockaddr_in providerAddr;
    struct hostent *providerHost;

    providerHost = gethostbyname(providerIp);
    if (providerHost == NULL)
    {
      fprintf(stderr, "Unknown host '%s'\n", providerIp);
      continue;
    }

    // limpa variavel do ip
    memset(providerIp, 0x0, MAX_PROVIDER_IP_LENGTH);

    providerAddr.sin_family = providerHost->h_addrtype;
    memcpy(
        (char *)&providerAddr.sin_addr.s_addr,
        providerHost->h_addr_list[0],
        providerHost->h_length);

    providerAddr.sin_port = htons(PROVIDER_PORT);

    messageIsSended = sendto(
        server,
        fileName,
        strlen(fileName) + 1,
        0,
        (struct sockaddr *)&providerAddr,
        sizeof(providerAddr));

    if (messageIsSended < 0)
    {
      fprintf(stderr, "Cannot send data\n");
      continue;
    }

    fprintf(stderr, "\n\nSalvando arquivo %s ...\n\n", fileName);

    char *basePath = "./tmp/";
    char *filePath = malloc(strlen(basePath) + strlen(fileName) + 1);
    strcpy(filePath, basePath);
    strcat(filePath, fileName);

    // limpa a variavel filename
    memset(fileName, 0x0, MAX_FILENAME_LENGTH);

    FILE *destinyFile = fopen(filePath, "wb");

    // // Recebe o buffer do arquivo
    char fileBuffer[MAX_FILE_BUFFER];
    memset(fileBuffer, 0x0, MAX_FILE_BUFFER);

    int providerSize = sizeof(providerAddr);
    while (1)
    {
      // receive
      recvfrom(
          server,
          fileBuffer,
          MAX_FILE_BUFFER,
          0,
          (struct sockaddr *)&providerAddr,
          &providerSize);

      fprintf(stderr, "Recebido %d bytes\n", strlen(fileBuffer));

      if (strcmp(fileBuffer, "END") == 0)
      {
        break;
      }

      // escreve o arquivo
      fwrite(fileBuffer, sizeof(char), strlen(fileBuffer), destinyFile);

      // limpa o buffer
      memset(fileBuffer, 0x0, MAX_FILE_BUFFER);


      // if (destinyFile == NULL)
      // {
      //   fprintf(stderr, "File transfer failed\n");
      //   exit(EXIT_FAILURE);
      // } // exit(EXIT_FAILURE);

      // for (int i = 0; i < strlen(fileBuffer); i++)
      // {
      //   fputc(fileBuffer[i], destinyFile);
      // }

      // // process
      // if (recvFile(fileBuffer, MAX_FILE_BUFFER))
      // {
      //   break;
      // }
    }

    fclose(destinyFile);

    fprintf(stderr, "Arquivo recebido com sucesso %s\n", fileBuffer);

    // int fileBufferWasReceived = recvfrom(
    //     server,
    //     fileBuffer,
    //     10,
    //     0,
    //     (struct sockaddr *)&providerAddr,
    //     &providerSize);
    // if (fileBufferWasReceived < 0)
    // {
    //   fprintf(stderr, "Cannot receive data\n");
    //   continue;
    // }

    // Salva o arquivo

    printf("Save file successfully.\n");
    // fclose(destinyFile);

    // char *path = "./files";
    // char *pathWithFileName = malloc(strlen(path) + strlen(fileName) + 1);
    // strcpy(pathWithFileName, path);

    // printf("\n\n%s\n\n", pathWithFileName);

    // FILE *destinyFile = fopen(pathWithFileName, "wb");

    // if (destinyFile == NULL)
    // {
    //   fprintf(stderr, "File transfer failed\n");
    //   exit(EXIT_FAILURE);
    // } // exit(EXIT_FAILURE);

    // for (int i = 0; i < originFileLength; i++)
    // {
    //   fputc(fgetc(originFile), destinyFile);
    // }

    // printf("File copied successfully.\n");
    // fclose(destinyFile);
  }

  return 0;
}
