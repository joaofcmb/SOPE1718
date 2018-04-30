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
    printf("Format: %s <time_out> <num_wanted_seats> <pref_seat_list>", argv[0]);
  }

  char pid[WIDTH_PID], ansFIFO[WIDTH_PID+3];
  int numSeats = atoi(argv[1]);

  // Normalize arguments
  sprintf(pid, "%0*d", WIDTH_PID, getpid());
  // get each seat, normalize it, and concat it back

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
    perror("requests");
    exit(2);
  }

  // TODO write normalized args on requests fifo

  exit(0);
}
