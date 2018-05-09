#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "protocol.h"

// macro function to keep the code for the first detected error
#define SET_ERRCODE(errcode, n)   (errcode) = ((errcode) != 0) ? (n) : (errcode)

char request[WIDTH_REQUEST];

void *booth(void *max_seats)
{
  char pid[WIDTH_PID + 1], ansFIFO[3 + WIDTH_PID + 1];
  char seats[(WIDTH_SEAT + 1) * MAX_CLI_SEATS + 1], pref_seats[MAX_CLI_SEATS];
  int num_wanted_seats, num_prefered_seats, num_room_seats = *(int *)max_seats;
  int errcode = 0;

  while(/*still during open_time*/1)
  {
      // TODO Get request from buffer

      // Open Answer FIFO
      sprintf(ansFIFO, "ans%s", pid);
      int ansfd = open(ansFIFO, O_WRONLY);


      // Validate Request
      if (num_wanted_seats < 1)
        SET_ERRCODE(errcode, -4);
      if (num_wanted_seats > MAX_CLI_SEATS)
        SET_ERRCODE(errcode, -1);

      if (num_prefered_seats < num_wanted_seats || num_prefered_seats > MAX_CLI_SEATS)
        SET_ERRCODE(errcode, -2);

      for (int i = 0; i < num_prefered_seats; i++)
      {
        if (pref_seats[i] < 1 || pref_seats[i] > num_room_seats)
          SET_ERRCODE(errcode, -3);
      }


      // TODO Execute Request (when request is validated)

      // TODO Send Feedback to Client and destroy client FIFO
  }

  return NULL;
}

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    printf("Format: %s <num_room_seats> <num_ticket_offices> <open_time>", argv[0]);
    exit(-1);
  }

  int const numSeats = atoi(argv[1]);
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
    if (pthread_create(&booths[i], NULL, booth, (void *)&(numSeats)) != 0)
    {
      perror("Thread Create");
      exit(2);
    }
  }

  while(/*still during open_time*/1)
  {
    // TODO read requests from Server FIFO
  }

  // TODO Destroy Server Fifo

  // TODO Signal Threads to exit

  for (int i = 0; i < numBooths; i++)
  {
    if (pthread_join(booths[i], NULL) != 0)
    {
      perror("Thread Join");
      exit(3);
    }
  }

  exit(0);
}
