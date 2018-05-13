#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "protocol.h"

// macro function to keep the error code of the first detected error
#define SET_ERRCODE(errcode, n)   (errcode) = ((errcode) != 0) ? (n) : (errcode)


// Thread Termination Flag Vars and Functions
int term_flag = 0;
pthread_mutex_t term_mutex = PTHREAD_MUTEX_INITIALIZER;

void signalTermination()
{
  pthread_mutex_lock(&term_mutex);
  term_flag = 0;
  pthread_mutex_unlock(&term_mutex);
}

int isNotOver()
{
  pthread_mutex_lock(&term_mutex);
  int r = term_flag;
  pthread_mutex_unlock(&term_mutex);
  return r;
}


// Request Buffer Variables
char request[WIDTH_REQUEST];

pthread_mutex_t req_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t  empty_req_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t  full_req_cond = PTHREAD_COND_INITIALIZER;



// Room Seats Array Variables and Functions (-1 = seat available | 0 = pre-reserved | PID = seat taken)
typedef struct
{
  int num; // Set in initialize_seats once and keeps that same value (No Sync Needed)
  int position[MAX_ROOM_SEATS];
  pthread_mutex_t mutex[MAX_ROOM_SEATS];
} Seat;
Seat seats;


int initialize_seats(Seat *seats, int numSeats)
{
  if (numSeats > MAX_ROOM_SEATS)  return -1;

  seats->num = numSeats;
  for (int i = 0; i < numSeats; i++)
  {
    seats->position[i] = -1;
    if (pthread_mutex_init(&(seats->mutex[i]), NULL) != 0)
      return -1;
  }
  return 0;
}

#define DELAY()

// Checks if it is free, if so it gets pre-reserved
int isSeatFree(Seat *seats, int seatNum)
{
  pthread_mutex_lock(&(seats->mutex[seatNum]));
    int value = seats->position[seatNum];
    if (value == -1)  seats->position[seatNum] = 0; // Pre-reserved

    DELAY();
  pthread_mutex_unlock(&(seats->mutex[seatNum]));

  return (value == -1);
}

// Assigns seat to Client
void bookSeat(Seat *seats, int seatNum, int clientId)
{
  pthread_mutex_lock(&(seats->mutex[seatNum]));
    seats->position[seatNum] = clientId;

    DELAY();
  pthread_mutex_unlock(&(seats->mutex[seatNum]));
}

// Frees Pre-reserved seat
void freeSeat(Seat *seats, int seatNum)
{
  pthread_mutex_lock(&(seats->mutex[seatNum]));
    seats->position[seatNum] = -1;

    DELAY();
  pthread_mutex_unlock(&(seats->mutex[seatNum]));
}



void *booth(void *max_seats)
{
  char curr_request[WIDTH_REQUEST], feedback[WIDTH_FEEDBACK], *token;
  char pid[WIDTH_PID + 1], ansFIFO[3 + WIDTH_PID + 1];

  int pref_seats[MAX_CLI_SEATS], reserved_seats[MAX_CLI_SEATS];

  int num_wanted_seats, num_prefered_seats, num_room_seats = *(int *)max_seats;
  int errcode = 0;

  while(isNotOver())
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
      strcpy(pid, token);
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

      // TODO Execute Request (if request is validated)
      if (errcode == 0)
      {
        int k = 0;
        for (int i = 0; i < num_prefered_seats; i++)
        {
          if (isSeatFree(&seats, pref_seats[i]))
          {
            reserved_seats[k] = pref_seats[i];
            k++;
          }
          if (k == num_wanted_seats)  break;
        }

        // Deal with reservations depending on ammount
        if (k == 0)
        {
          errcode = -6;
        }
        else if (k < num_wanted_seats)
        {
          errcode = -5;
          for (int i = 0; i < k; i++)
            freeSeat(&seats, reserved_seats[i]);
        }
        else
        {
          for (int i = 0; i < k; i++)
            bookSeat(&seats, reserved_seats[i], atoi(pid));
        }
      }

      // Open Answer FIFO
      sprintf(ansFIFO, "ans%s", pid);
      int ansfd = open(ansFIFO, O_WRONLY);
      if (ansfd < 0)
      {
        perror("ansfd");
        exit(11);
      }

      // Setup Feedback String for transmitting
      if (errcode == 0)
      {
        sprintf(feedback, "%0*d", WIDTH_SEAT, num_wanted_seats); // guarda o numero de lugares

        for(int i = 0; i < num_wanted_seats; i++)
        {
          char seat[WIDTH_SEAT + 2];
          sprintf(seat, " %0*d", WIDTH_SEAT, reserved_seats[i]);
          strcat(feedback, seat);
        }
      }
      else
      {
        sprintf(feedback, "%d", errcode);
      }

      #ifdef DEBUG
        printf("Feedback Debug: |%s|\n", feedback);
      #endif

      // Send Feedback to Client and close answer FIFO
      write(ansfd, feedback, WIDTH_FEEDBACK);
      close(ansfd);
  }

  return NULL;
}



int main(int argc, char* argv[])
{
  time_t initTime = time(NULL);

  if (argc < 4)
  {
    printf("Format: %s <num_room_seats> <num_ticket_offices> <open_time>", argv[0]);
    exit(-1);
  }

  int const numSeats = atoi(argv[1]);
  int const numBooths = atoi(argv[2]);
  int const openTime = atoi(argv[3]);
  pthread_t booths[numBooths];

  // Initialize Seats
  if (initialize_seats(&seats, numSeats) != 0)
  {
    if (numSeats > MAX_ROOM_SEATS)
    {
      printf("Number of Room Seats higher than MAX_ROOM_SEATS. Terminating.\n");
      exit(-2);
    }
    perror("Seats Initialization");
    exit(-3);
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

  // Create Server fifo for receiving client requests
  if (mkfifo("requests", 600) != 0)
  {
    perror("requests");
    exit(1);
  }

  // Open Server FIFO
  int requestfd = open("requests", O_RDONLY);
  if (requestfd < 0)
  {
    perror("Server: requests");
    exit(3);
  }

  while(difftime(time(NULL), initTime) < openTime)
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

  // Close and destroy Server Fifo
  close(requestfd);
  if (unlink("requests") < 0)
  {
    perror("requests");
    exit(4);
  }

  // Signal Threads to exit
  signalTermination();

  // Wait for threads to terminate
  for (int i = 0; i < numBooths; i++)
  {
    if (pthread_join(booths[i], NULL) != 0)
    {
      perror("Thread Join");
      exit(4);
    }
  }

  // Destroy Synchronization Structures
  pthread_mutex_destroy(&req_mutex);
  pthread_cond_destroy(&full_req_cond);
  pthread_mutex_destroy(&empty_req_cond);

  pthread_mutex_destroy(&term_mutex);

  for (int i = 0; i < numSeats; i++)
    pthread_mutex_destroy(&(seats->mutex[i]));

  exit(0);
}

  sprintf(serial,"%02d-%s-%02d: %s -%s\n", booths[i], PID, numSeats, seats, VALIDSEATS); //SUBSTITUIR VALIDSEATS por a variavel
  fprintf(f, serial);

  sprintf(serial,"%02d-%s-%02d: %s -MAX\n", booths[i], PID, numSeats, seats); //a quantidade de lugares pretendidos é superior ao máximo permitido
  fprintf(f, serial);

  sprintf(serial,"%02d-%s-%02d: %s -NST\n", booths[i], PID, numSeats, seats); //o número de identificadores dos lugares pretendidos não é válido
  fprintf(f, serial);

  sprintf(serial,"%02d-%s-%02d: %s -IID\n", booths[i], PID, numSeats, seats); //os identificadores dos lugares pretendidos não são válidos
  fprintf(f, serial);

  sprintf(serial,"%02d-%s-%02d: %s -ERR\n", booths[i], PID, numSeats, seats); //outros erros nos parâmetros
  fprintf(f, serial);

  sprintf(serial,"%02d-%s-%02d: %s -NAV\n", booths[i], PID, numSeats, seats); //pelo menos um dos lugares pretendidos não está disponível
  fprintf(f, serial);

  sprintf(serial,"%02d-%s-%02d: %s -FUL\n", booths[i], PID, numSeats, seats); //sala cheia
  fprintf(f, serial);

  sprintf(serial,"%02d-%s-%02d: %s -OUT\n", booths[i], PID, numSeats, seats); //esgotou o tempo
  fprintf(f, serial);


//booths[i]-pid-numSeats: seats           -lugar atribuido
