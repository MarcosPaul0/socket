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
#include <string.h> /* memset() */
#include <sys/time.h> /* select() */ 

#define REMOTE_SERVER_PORT 1500
#define MAX_MSG 100

/*
  argc = número de argumentos passsados na linha de comando
  argv[0] = ./nome_do_programa
  argv[1] = ip do servidor
  argv[2] = mensagem
*/
int main(int argc, char *argv[]) {
  
  int sd, rc, i;
  struct sockaddr_in cliAddr, remoteServAddr;
  struct hostent *h;

  /* check command line args */
  if(argc<3) {
    printf("usage : %s <server> <data1> ... <dataN> \n", argv[0]);
    //exit(1);
  }

  /* get server IP address (no check if input is IP address or DNS name */
  h = gethostbyname(argv[1]);
  if(h==NULL) {
    printf("%s: unknown host '%s' \n", argv[0], argv[1]);
    //exit(1);
  }

  printf("%s: sending data to '%s' (IP : %s) \n", argv[0], h->h_name,
	 inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));

  remoteServAddr.sin_family = h->h_addrtype;
  memcpy((char *) &remoteServAddr.sin_addr.s_addr, 
	 h->h_addr_list[0], h->h_length);
  remoteServAddr.sin_port = htons(REMOTE_SERVER_PORT);

  /* socket creation */
  sd = socket(AF_INET, SOCK_DGRAM, 0); // Cria um socket UDP
  if(sd<0) {
    printf("%s: cannot open socket \n",argv[0]);
    //exit(1);
  }
  
  /* bind any port */
  cliAddr.sin_family = AF_INET; // Família de protocolo - IPv4
  cliAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Edereço local utilizado pelo socket convertido pelo htonl
  cliAddr.sin_port = htons(0); // Porta local utilizada pelo socket convertida pelo htons
  
  rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
  if(rc<0) {
    printf("%s: cannot bind port\n", argv[0]);
    //exit(1);
  }


  /* send data */
  for(i=2;i<argc;i++) {
    rc = sendto(sd, argv[i], strlen(argv[i])+1, 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));

    if(rc<0) {
      printf("%s: cannot send data %d \n",argv[0],i-1);
      close(sd);
      //exit(1);
    }

  }
  
  return 1;

}

