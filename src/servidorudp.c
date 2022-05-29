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
    fprintf(
      stderr,
      "Connection to database failed: %s\n",
      PQerrorMessage(connection)
    );
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

// Insere novo usuário do programa
void insertNewUser(PGconn *connection, const char *ip)
{
  char *sql = "INSERT INTO users (ip) VALUES ($1)";
  PGresult *result = PQexecParams(connection, sql, 1, NULL, &ip, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    fprintf(
      stderr,
      "Insert record failed: %s\n",
      PQerrorMessage(connection)
    );
    disconnect(connection);
  }

  PQclear(result);
}

// Insere novo arquivo ao usuário
void insertNewFileToUser(PGconn *connection, const char *ip, const char *name)
{
  const char *params[2];
  params[0] = name;
  params[1] = ip;

  PGresult *result = PQexec(connection, "BEGIN");

  char *sql = "INSERT INTO files(name) VALUES($1)";
  result = PQexecParams(connection, sql, 1, NULL, &name, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    fprintf(
      stderr,
      "Insert record failed: %s\n",
      PQerrorMessage(connection)
    );
    disconnect(connection);
  }

  sql = "INSERT INTO user_file(user_id, file_id) VALUES((SELECT(users.id) FROM users WHERE users.ip = $2), (SELECT(files.id) FROM files WHERE files.name = $1))";
  result = PQexecParams(connection, sql, 2, NULL, params, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    fprintf(
      stderr,
      "Insert record failed: %s\n",
      PQerrorMessage(connection)
    );
    disconnect(connection);
  }

  result = PQexec(connection, "COMMIT");

  PQclear(result);
}

// Busca o usuário pelo ip
char *findUserByIp(PGconn *connection, const char *ip)
{
  char *ipFound;
  char *sql = "SELECT (users.ip) FROM users WHERE ip = $1";
  PGresult *result = PQexecParams(connection, sql, 1, NULL, &ip, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    fprintf(
      stderr,
      "Select record failed: %s\n",
      PQerrorMessage(connection)
    );
    disconnect(connection);
  }

  if (PQntuples(result) == 0)
  {
    PQclear(result);
    return NULL;
  }

  ipFound = PQgetvalue(result, 0, 0);
  PQclear(result);

  return ipFound;
}

// Busca o usuário que possui o arquivo
char *findReceiverUserIpByFileName(PGconn *connection, const char *name)
{
  char *ipFound;
  char *sql = "SELECT (users.ip) FROM files INNER JOIN user_file ON  files.name = $1 AND files.id = user_file.file_id INNER JOIN users ON user_file.user_id = users.id";

  PGresult *result = PQexecParams(connection, sql, 1, NULL, &name, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    fprintf(
      stderr,
      "Select record failed: %s\n",
      PQerrorMessage(connection)
    );
    disconnect(connection);
  }

  if (PQntuples(result) == 0)
  {
    PQclear(result);
    return NULL;
  }

  ipFound = PQgetvalue(result, 0, 0);
  PQclear(result);

  return ipFound;
}

int main(int argc, char *argv[])
{
  int server, portIsBind, fileNameWasReceived, cliLen;
  struct sockaddr_in cliAddr, servAddr;
  PGconn *pgConnection = connection();

  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(LOCAL_SERVER_PORT);

  /* socket creation */
  server = socket(AF_INET, SOCK_DGRAM, 0); // Cria um socket UDP

  if (server < 0)
  {
    fprintf(
      stderr,
      "%s: cannot open socket \n",
      argv[0]
    );
    exit(1);
  }

  /* bind local server port */

  portIsBind = bind( // associa porta ao socket
    server,
    (struct sockaddr *)&servAddr,
    sizeof(servAddr)
  );

  if (portIsBind < 0)
  {
    fprintf(
      stderr,
      "Cannot bind port number %d \n",
      LOCAL_SERVER_PORT
    );
    exit(1);
  }

  fprintf(
    stderr,
    "Waiting for data on port UDP %u\n",
    LOCAL_SERVER_PORT
  );

  /* server infinite loop */
  while (1)
  {
    /* init buffer */
    char fileName[MAX_MSG];
    memset(fileName, 0x0, MAX_MSG);

    /* receive message */
    cliLen = sizeof(cliAddr);
    fileNameWasReceived = recvfrom(
      server,
      fileName,
      MAX_MSG,
      0,
      (struct sockaddr *)&cliAddr,
      &cliLen
    );

    if (fileNameWasReceived < 0)
    {
      fprintf(stderr, "Cannot receive data \n");
      continue; // reestarta o loop
    }

    // Se o usuário não existe, insere no banco
    char *receiverUserIp, *providerUserIp;

    receiverUserIp = findUserByIp(pgConnection, inet_ntoa(cliAddr.sin_addr));

    if (receiverUserIp == NULL)
    {
      insertNewUser(pgConnection, inet_ntoa(cliAddr.sin_addr));
      receiverUserIp = inet_ntoa(cliAddr.sin_addr);
    }

    // Procura o usuário que possui o arquivo informado
    providerUserIp = findReceiverUserIpByFileName(pgConnection, fileName);

    if (providerUserIp == NULL)
    {
      fprintf(stderr, "File does not exists in our database\n");
      sendto(
        server,
        "",
        strlen("") + 1,
        0,
        (struct sockaddr *)&cliAddr,
        sizeof(cliAddr)
      );
      continue;
    }

    // Retorna o endereço de ip do usuário que possui o arquivo
    sendto(
      server,
      providerUserIp,
      strlen(providerUserIp) + 1,
      0,
      (struct sockaddr *)&cliAddr,
      sizeof(cliAddr)
    );

    // // Caso a tranferência seja realizada com sucesso, registra que o usuário também possui o arquivo
    // insertNewFileToUser(pgConnection, receiverUserIp, fileName);
  } /* end of server infinite loop */

  return 0;
}
