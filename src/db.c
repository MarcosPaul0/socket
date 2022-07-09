#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h> // PostgreSQL
#include <netdb.h>

#include "db.h"

PGconn *connection()
{
  PGconn *connection = PQconnectdb("host=localhost password=123 user=postgres dbname=tracker_server");

  if (PQstatus(connection) == CONNECTION_BAD)
  {
    printf(
        "Connection to database failed: %s\n",
        PQerrorMessage(connection));
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
    printf(
        "Insert record failed: %s\n",
        PQerrorMessage(connection));
    disconnect(connection);
  }

  PQclear(result);
}

void insertNewFile(PGconn *connection, const char *name)
{
  char *sql = "INSERT INTO files (name) VALUES ($1)";
  PGresult *result = PQexecParams(connection, sql, 1, NULL, &name, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    printf(
        "Insert record failed: %s\n",
        PQerrorMessage(connection));
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

  char *sql = "INSERT INTO user_file(user_id, file_id) VALUES((SELECT(users.id) FROM users WHERE users.ip = $2), (SELECT(files.id) FROM files WHERE files.name = $1))";
  result = PQexecParams(connection, sql, 2, NULL, params, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    printf(
        "Insert record failed: %s\n",
        PQerrorMessage(connection));
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
    printf(
        "Select record failed: %s\n",
        PQerrorMessage(connection));
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
  char *userIpFound;
  char *sql = "SELECT (users.ip) FROM files INNER JOIN user_file ON  files.name = $1 AND files.id = user_file.file_id INNER JOIN users ON user_file.user_id = users.id LIMIT 1";

  PGresult *result = PQexecParams(connection, sql, 1, NULL, &name, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    printf(
        "Select record failed: %s\n",
        PQerrorMessage(connection));
    disconnect(connection);
  }

  if (PQntuples(result) == 0)
  {
    PQclear(result);
    return NULL;
  }

  userIpFound = PQgetvalue(result, 0, 0);
  PQclear(result);

  return userIpFound;
}

// Busca o arquivo pelo ip do usuário
char *findFileByUserIp(PGconn *connection, const char *name, const char *ip) {
  char *fileNameFound;
  char *sql = "SELECT (files.name) FROM files INNER JOIN user_file ON  files.id = user_file.file_id INNER JOIN users ON user_file.user_id = users.id WHERE users.ip = $2 AND files.name = $1 LIMIT 1";

  const char *params[2];
  params[0] = name;
  params[1] = ip;

  PGresult *result = PQexecParams(connection, sql, 2, NULL, params, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    printf(
        "Select record failed: %s\n",
        PQerrorMessage(connection));
    disconnect(connection);
  }

  if (PQntuples(result) == 0)
  {
    PQclear(result);
    return NULL;
  }

  fileNameFound = PQgetvalue(result, 0, 0);
  PQclear(result);

  return fileNameFound;
}