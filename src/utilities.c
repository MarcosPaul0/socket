#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "utilities.h"

int checkSumInHex(char *packetContent, int length)
{
  int sum = packetContent[0];

  for (int i = 1; i < length; i++)
  {
    sum += packetContent[i];

    if (sum > 255)
    {
      sum -= 255;
    }
  }

  return sum;
}

int isValidChecksum(char *packetContent, int length, char *inverseChecksum)
{
  int checksum;

  checksum = checkSumInHex(packetContent, length);

  if (checksum + (int)strtol(inverseChecksum, NULL, 16) != 0xFF)
  {
    return 0;
  }

  return 1;
}
