#include <libpq-fe.h> // PostgreSQL

PGconn *connection();

void disconnect(PGconn *connection);

// Insere novo usuário do programa
void insertNewUser(PGconn *connection, const char *ip);

// Insere novo arquivo ao usuário
void insertNewFileToUser(PGconn *connection, const char *ip, const char *name);

// Busca o usuário pelo ip
char *findUserByIp(PGconn *connection, const char *ip);

// Busca o usuário que possui o arquivo
char *findReceiverUserIpByFileName(PGconn *connection, const char *name);

char *findFileByUserIp(PGconn *connection, const char *name, const char *ip);