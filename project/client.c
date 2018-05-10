#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "protocol.h"

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    printf("Format: %s <time_out> <num_wanted_seats> <pref_seat_list>\n", argv[0]);
    exit(-1);
  }

  char *token;
  char pid[WIDTH_PID + 1], numSeats[WIDTH_SEAT + 1];
  char seats[(WIDTH_SEAT + 1) * MAX_CLI_SEATS + 1];

  char ansFIFO[3 + WIDTH_PID + 1], serial[WIDTH_REQUEST];

  // Normalize arguments (PID and seats)
  sprintf(pid, "%0*d", WIDTH_PID, getpid());
  sprintf(numSeats, "%0*d", WIDTH_SEAT, atoi(argv[2]));

  strcpy(seats, "");
  token = strtok(argv[3], " ");
  while(token != NULL)
  {
    // normalize each seat
    char seat[WIDTH_SEAT + 2];
    sprintf(seat, " %0*d", WIDTH_SEAT, atoi(token));
    strcat(seats, seat);

    token = strtok(NULL, " ");
  }

  // serialize request for sending via FIFO (seats has trailing space)
  sprintf(serial, "%s %s%s", pid, numSeats, seats);

  #ifdef DEBUG
    printf("DEBUG Client Serial: |%s|\n", serial);
  #endif

  // Create client fifo for server feedback
  sprintf(ansFIFO, "ans%s", pid);
  if (mkfifo(ansFIFO, 600) != 0)
  {
    perror(ansFIFO);
    exit(1);
  }

  // open server fifo and send request
  int requestfd = open("requests", O_WRONLY | O_APPEND);
  if (requestfd < 0)
  {
    perror("Client: requests");
    exit(2);
  }
  write(requestfd, serial, WIDTH_REQUEST);

  // TODO Close connection to server Fifo

  // TODO Wait for Server feedback and act accordingly

  // TODO Destroy Client FIFO

  exit(0);
}
