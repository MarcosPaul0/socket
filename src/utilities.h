#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#define MAX_FILE_BUFFER 32

struct package
{
  int sequenceNumber;
  int checkSum;
  char data[MAX_FILE_BUFFER];
  int dataSize;
};

int checkSumInHex(char *packetContent, int length);
int isValidChecksum(char *packetContent, int length, char *inverseChecksum);