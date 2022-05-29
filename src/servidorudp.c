/*
  Compilar - gcc src/servidorudp.c -o servidorudp -lpq
  Executar - ./servidorudp
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
#include <libpq-fe.h> // PostgreSQL

#define LOCAL_SERVER_PORT 1500
#define MAX_MSG 100

PGconn *connection()
{
  PGconn *connection = PQconnectdb("host=localhost password=123 user=postgres dbname=tracker_server");

  if (PQstatus(connection) == CONNECTION_BAD)
  {
    fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(connection));
    PQfinish(connection);
    exit(1);
  }

  return connection;
}

void disconnect(PGconn *connection) 
{
    PQfinish(connection);
    exit(1);
}

// void insertNewUser(PGconn *connection, char *ip)
// {
//   char *sql = "INSERT INTO users (ip) VALUES ($1)";
//   PGresult *result = PQexecParams(connection, sql, 1, NULL, ip, NULL, NULL, 0);

//   if (PQresultStatus(result) != PGRES_COMMAND_OK)
//   {
//     fprintf(stderr, "Insert record failed: %s\n", PQerrorMessage(connection));
//     disconnect(connection);
//   }

//   PQclear(result);
// }

// void insertNewFileToUser(PGconn *connection, char *user_id, char *name, char *type)
// {
//   char *sql = "INSERT INTO files (name, type) VALUES ($1, $2)";

//   PGresult *result = PQexecParams(connection, sql, 2, NULL, &name, &type, NULL, 0);

//   if (PQresultStatus(result) != PGRES_COMMAND_OK)
//   {
//     fprintf(stderr, "Insert record failed: %s\n", PQerrorMessage(connection));
//     disconnect(connection);
//   }

//   PQclear(result);
// }

// void insertNewFileToUser(PGconn *connection, char *user_id, char *file_id)
// {
//   char *sql = "INSERT INTO user_file (user_id, file_id) VALUES ($1, $2)";

//   PGresult *result = PQexecParams(connection, sql, 2, NULL, &user_id, &file_id, NULL, 0);

//   if (PQresultStatus(result) != PGRES_COMMAND_OK)
//   {
//     fprintf(stderr, "Insert record failed: %s\n", PQerrorMessage(connection));
//     disconnect(connection);
//   }

//   PQclear(result);
// }

// void findUserByIpAnsInsertIfNotExists(PGconn *connection, char *ip)
// {
//   char *sql = "SELECT * FROM users WHERE ip = $1";
//   PGresult *result = PQexecParams(connection, sql, 1, NULL, &ip, NULL, NULL, 0);

//   if (PQresultStatus(result) != PGRES_TUPLES_OK)
//   {
//     fprintf(stderr, "Select record failed: %s\n", PQerrorMessage(connection));
//     disconnect(connection);
//   }

//   if (PQntuples(result) == 0)
//   {
//     insertNewUser(connection, ip);
//   }

//   PQclear(result);
// }

char* findUserIpByFileName(PGconn *connection, const char *name)
{
  char *sql = "SELECT (users.ip) FROM files INNER JOIN user_file ON  files.name = $1 AND files.id = user_file.file_id INNER JOIN users ON user_file.user_id = users.id";

  PGresult *result = PQexecParams(connection, sql, 1, NULL, &name, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    fprintf(stderr, "Select record failed: %s\n", PQerrorMessage(connection));
    disconnect(connection);
  }

  if (PQntuples(result) == 0)
  {
    return NULL;
  }

  return PQgetvalue(result, 0, 0);
}

int main(int argc, char *argv[])
{
  int server, portIsBind, messageWasReceived, cliLen;
  struct sockaddr_in cliAddr, servAddr;
  char message[MAX_MSG];
  char *user_ip;
  PGconn *pgConnection = connection();

  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(LOCAL_SERVER_PORT);

  /* socket creation */
  server = socket(AF_INET, SOCK_DGRAM, 0); // Cria um socket UDP

  if (server < 0)
  {
    printf("%s: cannot open socket \n", argv[0]);
    exit(1);
  }

  /* bind local server port */

  portIsBind = bind( // associa porta ao socket
      server,
      (struct sockaddr *)&servAddr,
      sizeof(servAddr));

  if (portIsBind < 0)
  {
    printf(
        "Cannot bind port number %d \n",
        LOCAL_SERVER_PORT);
    exit(1);
  }

  printf(
      "Waiting for data on port UDP %u\n",
      LOCAL_SERVER_PORT);

  /* server infinite loop */
  while (1)
  {
    /* init buffer */
    memset(message, 0x0, MAX_MSG);

    /* receive message */
    cliLen = sizeof(cliAddr);
    messageWasReceived = recvfrom(
        server,
        message,
        MAX_MSG,
        0,
        (struct sockaddr *)&cliAddr,
        &cliLen);

    if (messageWasReceived < 0)
    {
      printf("Cannot receive data \n");
      continue; // reestarta o loop
    }

    /* print received message */
    // printf(
    //     "From %s:UDP%u : %s \n",
    //     inet_ntoa(cliAddr.sin_addr),
    //     ntohs(cliAddr.sin_port),
    //     message);

    user_ip = findUserIpByFileName(pgConnection, message);
    printf("%s\n", user_ip);
  } /* end of server infinite loop */

  return 0;
}
