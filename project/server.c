#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "protocol.h"

// macro function to keep the error code of the first detected error
#define SET_ERRCODE(errcode, n)   (errcode) = ((errcode) != 0) ? (n) : (errcode)

char request[WIDTH_REQUEST];

pthread_mutex_t req_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t  empty_req_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t  full_req_cond = PTHREAD_COND_INITIALIZER;

void *booth(void *max_seats)
{
  char curr_request[WIDTH_REQUEST], *token;
  char ansFIFO[3 + WIDTH_PID + 1];

  int pref_seats[MAX_CLI_SEATS]; // contains list of prefered seats

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

      // Parse request
      token = strtok(curr_request, " ");  // Client PID
      sprintf(ansFIFO, "ans%s", token);
      token = strtok(NULL, " ");          // num_wanted_seats
      num_wanted_seats = atoi(token);

      for (num_prefered_seats = 0; token != NULL; num_prefered_seats++)
      {
        token = strtok(NULL, " ");
        pref_seats[num_prefered_seats] = atoi(token);
      }

      #ifdef DEBUG
        printf("DEBUG Booth Request: %s %d", ansFIFO, num_wanted_seats);
        for (int i = 0; i < num_prefered_seats; i++)
          printf(" %d", pref_seats[i]);
        printf("\n");
      #endif

      // Open Answer FIFO
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
    char tmp_request[WIDTH_REQUEST];
    read (requestfd, tmp_request, WIDTH_REQUEST);

    pthread_mutex_lock(&req_mutex);
    while (request != NULL)
      pthread_cond_wait(&empty_req_cond, &req_mutex);

    strcpy(request, tmp_request);
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
