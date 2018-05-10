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

pthread_mutex_t req_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t  empty_req_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t  full_req_cond = PTHREAD_COND_INITIALIZER;

void *booth(void *max_seats)
{
  char curr_request[WIDTH_REQUEST]; // holds current request (serialized)

  char pid[WIDTH_PID + 1], ansFIFO[3 + WIDTH_PID + 1];

  // contains the list of prefered seats as a string "XXX1 XXX2 ... XXXN"
  char seats[(WIDTH_SEAT + 1) * MAX_CLI_SEATS + 1];
  // contains the list of prefered seats as an array of ints
  int pref_seats[MAX_CLI_SEATS];

  int num_wanted_seats, num_prefered_seats, num_room_seats = *(int *)max_seats;
  int errcode = 0;

  while(/*still during open_time*/1)
  {
      // Get request from buffer
      pthread_mutex_lock(&req_mutex);
      while (request == NULL)
        pthread_cond_wait(&full_req_cond, &req_mutex);

      strcpy(curr_request, request);
      strcpy(request, "");
      pthread_cond_signal(&empty_req_cond);
      pthread_mutex_unlock(&req_mutex);

      // TODO Parse request

      // Open Answer FIFO
      sprintf(ansFIFO, "ans%s", pid);
      int ansfd = open(ansFIFO, O_WRONLY);
      if (ansfd < 0)
      {
        perror("ansfd");
        exit(11);
      }

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

      // TODO Send Feedback to Client and destroy answer FIFO
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

  // Open Server FIFO
  int requestfd = open("requests", O_RDONLY);
  if (requestfd < 0)
  {
    perror("Server: requests");
    exit(3);
  }

  while(/*still during open_time*/1)
  {
    // Read upcomming requests and put in buffer
    pthread_mutex_lock(&req_mutex);
    while (request != NULL)
      pthread_cond_wait(&empty_req_cond, &req_mutex);

    read(requestfd, request, WIDTH_REQUEST);
    pthread_cond_signal(&full_req_cond);
    pthread_mutex_unlock(&req_mutex);
  }

  // TODO Close and destroy Server Fifo

  // TODO Signal Threads to exit

  // Wait for threads to terminate
  for (int i = 0; i < numBooths; i++)
  {
    if (pthread_join(booths[i], NULL) != 0)
    {
      perror("Thread Join");
      exit(4);
    }
  }

  exit(0);
}
