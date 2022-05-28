/*
  Instruções para compilação e execução
  Disciplina COM240 - Redes de Computadores
  Professor Bruno Guazzelli Batista
  Compilar - gcc clienteudp.c -o clienteudp
  Executar - ./clienteudp 127.0.0.1 mensagem
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

#define REMOTE_SERVER_PORT 1500
#define MAX_MSG 100
#define CRAWLER_SERVER_IP "127.0.0.1"

/*
  argc = número de argumentos passsados na linha de comando
  argv[0] = ./nome_do_programa
  argv[1] = ip do servidor
  argv[2] = mensagem
*/
int main()
{
  int server, portIsBind, messageIsSended, i;
  struct sockaddr_in clientAddr, remoteServerAddr;
  struct hostent *host;

  clientAddr.sin_family = AF_INET;                // Família de protocolo - IPv4
  clientAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Edereço local utilizado pelo socket convertido pelo htonl
  clientAddr.sin_port = htons(0);                 // Porta local utilizada pelo socket convertida pelo htons

  server = socket(AF_INET, SOCK_DGRAM, 0); // Cria um socket UDP
  if (server < 0)
  {
    printf("Cannot open socket\n");
    exit(1);
  }

  // Bind a port number to the socket
  portIsBind = bind(
    server,
    (struct sockaddr *)&clientAddr,
    sizeof(clientAddr)
  );

  if (portIsBind < 0)
  {
    printf("Cannot bind port\n");
    exit(1);
  }

  /* get server IP address (no check if input is IP address or DNS name */
  host = gethostbyname(CRAWLER_SERVER_IP);
  if (host == NULL)
  {
    printf("Unknown host '%s'\n", CRAWLER_SERVER_IP);
    exit(1);
  }

  // printf("%s: sending data to '%s' (IP : %s) \n", argv[0], h->h_name,
  //        inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));

  remoteServerAddr.sin_family = host->h_addrtype;
  memcpy(
    (char *)&remoteServerAddr.sin_addr.s_addr,
    host->h_addr_list[0],
    host->h_length
  );
  
  remoteServerAddr.sin_port = htons(REMOTE_SERVER_PORT);

  // O usuário insere o arquivo desejado
  printf("Digite o nome do arquivo: ");
  char fileName[100];
  scanf("%s", fileName);

  /* send data */
  messageIsSended = sendto(
    server,
    fileName,
    strlen(fileName) + 1,
    0,
    (struct sockaddr *)&remoteServerAddr,
    sizeof(remoteServerAddr)
  );

  if (messageIsSended < 0)
  {
    printf("Cannot send data %d \n", i - 1);
    close(server);
    exit(1);
  }

  return 0;
}
