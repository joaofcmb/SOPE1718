#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

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
  char seats[(WIDTH_SEAT + 1) * MAX_CLI_SEATS + 1] = "", seat[WIDTH_SEAT + 2];

  char ansFIFO[3 + WIDTH_PID + 1], serial[WIDTH_REQUEST], feedback[WIDTH_FEEDBACK];

  // Normalize arguments (PID and seats)
  sprintf(pid, "%0*d", WIDTH_PID, getpid()); // guarda o pid
  sprintf(numSeats, "%0*d", WIDTH_SEAT, atoi(argv[2])); // guarda o numero de lugares

  token = strtok(argv[3], " ");
  while(token != NULL)
  {
    // normalize each seat
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
  if (mkfifo(ansFIFO, 0600) != 0)
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

  // Close connection to server Fifo
  if (close(requestfd) < 0)
  {
    perror("Client: requests");
    exit(3);
  }

  #ifdef DEBUG
    printf("DEBUG: Request sent\n");
  #endif

  // Open Client FIFO
  int ansfd = open(ansFIFO, O_RDONLY | O_NONBLOCK);

  // Wait for Server feedback and treat data
  int errcode = 0, num_reserved_seats, reserved_seats[MAX_CLI_SEATS];

  int timeout = atoi(argv[1]), r = -1;
  time_t initTime = time(NULL);

  while (difftime(time(NULL), initTime) < timeout && r <= 0)
    r = read(ansfd, feedback, WIDTH_FEEDBACK);

  #ifdef DEBUG
    printf("Received feedback: |%s|\n", feedback);
  #endif

  if (r == 0)
  {
    printf("Nothing read\n");
    exit(4);
  }
  if (r == -1)
  {
      if (errno == EAGAIN) // timeout occured
        errcode = -7;
      else
      {
        perror(ansFIFO);
        exit(4);
      }
  }
  else
  {
    token = strtok(feedback, " ");  // Errorcode / numSeats
    num_reserved_seats = atoi(token);

    if (num_reserved_seats < 0)
    {
      errcode = num_reserved_seats;
    }
    else
    {
      for (int i = 0; i < num_reserved_seats; i++)
      {
        token = strtok(NULL, " ");
        reserved_seats[i] = atoi(token);
      }
    }
  }

  #ifdef DEBUG
  printf("DEBUG: Feedback processed\n");
  #endif
  // Close and destroy the client fifo
  close(ansfd);
  if (unlink(ansFIFO) < 0)
  {
    perror(ansFIFO);
    exit(5);
  }


  // Open data files
  int logfd = open("clog.txt", O_WRONLY | O_APPEND | O_CREAT, 0600);
  if (logfd < 0)
  {
    perror("clog.txt");
    exit(11);
  }

  int bookfd = open ("cbook.txt", O_WRONLY | O_APPEND | O_CREAT, 0600);
  if (bookfd < 0)
  {
    perror("cbook.txt");
    exit(12);
  }


  // Insert data
  char line[WIDTH_PID + WIDTH_XXNN + WIDTH_SEAT + 3], xxnn[WIDTH_XXNN + 1];

  if (errcode < 0)
  {
    sprintf(line, "%s %s\n", pid, EMSG(errcode));
    write(logfd, line, strlen(line));
  }
  else
  {
    for (int i = 1; i <= num_reserved_seats; i++)
    {
      sprintf(xxnn, "%0*d.%0*d", WIDTH_XXNN / 2, i, WIDTH_XXNN / 2, num_reserved_seats);
      sprintf(seat, "%0*d", WIDTH_SEAT, reserved_seats[i]);

      sprintf(line, "%s %s %s\n", pid, xxnn, seat);
      write(logfd, line, strlen(line));

      sprintf(line, "%s\n", seat);
      write(bookfd, line, strlen(line));
    }
  }

  // Close data files
  if (close(logfd) < 0)
  {
    perror("clog.txt");
    exit(13);
  }

  if (close(bookfd) < 0)
  {
    perror("cbook.txt");
    exit(14);
  }


  exit(0);
}
