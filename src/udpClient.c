/*
  Compilar - gcc src/udpClient.c src/utilities.c -o udpClient -lm
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
#include <math.h>

#include "utilities.h"

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
    printf("Nao foi possivel definir o socket\n");
    exit(1);
  }

  // Bind a port number to the socket
  portIsBind = bind(
      server,
      (struct sockaddr *)&clientAddr,
      sizeof(clientAddr));

  if (portIsBind < 0)
  {
    printf("Nao foi possivel definir a porta\n");
    exit(1);
  }

  printf("Executando na porta %d\n", CLIENT_PORT);

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

  while (1)
  {
    // O usuário insere o arquivo desejado
    printf("Digite o nome do arquivo: ");
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
      printf("Nao foi possivel enviar o nome do arquivo\n");
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
      printf("Nao foi possivel receber o endereço do provedor\n");
      continue;
    }

    if (strcmp(providerIp, "404") == 0)
    {
      printf("Arquivo nao existe no banco de dados\n");  
      close(server);
      continue;
    }

    printf("\n\nO arquivo %s esta disponível no cliente %s\n", fileName, providerIp);
    printf("Fazendo conexao com o cliente %s...\n\n", providerIp);

    // Solicita o envio do arquivo para o usuário provedor
    // Envia o nome do arquivo requerido
    struct sockaddr_in providerAddr;
    struct hostent *providerHost;

    providerHost = gethostbyname(providerIp);
    if (providerHost == NULL)
    {
      printf("Nao foi possivel definir um host '%s'\n", providerIp);
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
      printf("Nao foi possivel informar o nome do arquivo\n");
      continue;
    }

    // Recebe o tamanho do arquivo
    int fileSize;

    recvfrom(
        server,
        &fileSize,
        sizeof(fileSize),
        0,
        (struct sockaddr *)&providerAddr,
        &trackerSize);

    printf("Arquivo %s com tamanho %d\n", fileName, fileSize);

    printf("\n\nSalvando arquivo %s...\n\n", fileName);

    char *basePath = "./tmp/";
    char *filePath = malloc(strlen(basePath) + strlen(fileName) + 1);
    strcpy(filePath, basePath);
    strcat(filePath, fileName);

    FILE *destinyFile = fopen(filePath, "wb");

    if (destinyFile == NULL)
    {
      printf("Nao foi possivel abrir o arquivo %s\n", filePath);
      continue;
    }

    int providerSize = sizeof(providerAddr);

    int packetQuantity = fileSize / MAX_FILE_BUFFER;
    int lastPacketSize = fileSize % MAX_FILE_BUFFER;

    printf("Quantidade %d\n", packetQuantity);
    printf("Tamanho %d\n", lastPacketSize);

    //  Armazena o ultimo numero de sequencia
    int lastSequenceNumber = -1;

    float totalReceived = 0;
    while (1)
    {
      struct package package;

      // Recebe o pacote
      recvfrom(
          server,
          &package,
          sizeof(package),
          0,
          (struct sockaddr *)&providerAddr,
          &providerSize);

      // Se for o ultimo pacote
      if (strcmp(package.data, "END") == 0)
      {
        break;
      }

      //  Verifica a quantidade de caracteres
      int sequenceNumberSize = package.sequenceNumber < 10 ? 1 : floor(log10(package.sequenceNumber)) + 1;

      //  Prepara a resposta
      struct package response;
      response.dataSize = sequenceNumberSize + 4;

      //  Verifica o checksum
      int checkSum = checkSumInHex(package.data, package.dataSize);

      if (checkSum + package.checkSum != 0xFF)
      {
        // printf("Checksum invalido\n");

        //  Envia o NCK
        memset(response.data, 0x0, MAX_FILE_BUFFER);
        memcpy(response.data, "NCK ", 4);
        sprintf(response.data + 4, "%d", package.sequenceNumber);

        sendto(
            server,
            &response,
            sizeof(response),
            0,
            (struct sockaddr *)&providerAddr,
            sizeof(providerAddr));

        continue;
      }

      // printf("Checksum valido\n");

      //  Envia o ACK
      memset(response.data, 0x0, MAX_FILE_BUFFER);
      memcpy(response.data, "ACK ", 4);

      sprintf(response.data + 4, "%d", package.sequenceNumber);

      sendto(
          server,
          &response,
          sizeof(response),
          0,
          (struct sockaddr *)&providerAddr,
          sizeof(providerAddr));

      //  Verifica se o numero de sequencia é o mesmo
      if (lastSequenceNumber == package.sequenceNumber)
      {
        printf("Numero de sequencia invalido\n");
        continue;
      }

      lastSequenceNumber = package.sequenceNumber;

      //  Grava o pacote no arquivo
      fwrite(package.data, 1, package.dataSize, destinyFile);

      totalReceived += floor(package.dataSize);
      printf("\n%.2f\% recebido\n", (totalReceived / fileSize) * 100);
    }
    
    fclose(destinyFile);

    // Avisa para o servidor rastreador que recebeu o arquvo
    messageIsSended = sendto(
        server,
        fileName,
        strlen(fileName),
        0,
        (struct sockaddr *)&trackerServerAddr,
        sizeof(trackerServerAddr));

    if (messageIsSended < 0)
    {
      printf("Nao foi possivel informar que o arquivo foi recebido\n");
      continue;
    }

    // limpa variavel do nome do arquivo
    memset(fileName, 0x0, MAX_FILENAME_LENGTH);

    printf("Arquivo recebido com sucesso\n");

    printf("Arquivo salvo com sucesso\n");
  }

  return 0;
}
