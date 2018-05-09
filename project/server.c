#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#include "protocol.h"

char request[WIDTH_REQUEST];

void *booth(void *open_time)
{
  return NULL;
}

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    printf("Format: %s <num_room_seats> <num_ticket_offices> <open_time>", argv[0]);
    exit(-1);
  }

  int totalSeats = atoi(argv[1]);
  int numBooths = atoi(argv[2]);
  pthread_t booths[numBooths];

  // Create Server fifo for receiving client requests
  if (mkfifo("requests", 600) != 0)
  {
    perror("requests");
    exit(1);
  }

  // Create booth threads
  for (int i = 0; i < numBooths; i++)
  {
    if (pthread_create(&booths[i], NULL, booth, (void *)argv[3]) != 0)
    {
      perror(NULL);
      exit(1);
    }
  }

  while(/*still during open_time*/0)
  {
    // TODO read requests from Server FIFO
  }

  // TODO Destroy Server Fifo and Booths after the requested time

  exit(0);
}
