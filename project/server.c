#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "protocol.h"

// macro function to keep the error code of the first detected error
#define SET_ERRCODE(errcode, n)   (errcode) = ((errcode) != 0) ? (n) : (errcode)


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

int takenSeats;

pthread_mutex_t taken_mutex = PTHREAD_MUTEX_INITIALIZER;

int getTakenSeats()
{
  pthread_mutex_lock(&taken_mutex);
  int r = takenSeats;
  pthread_mutex_unlock(&taken_mutex);
  return r;
}

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

  pthread_mutex_lock(&taken_mutex);
  takenSeats = 0;
  pthread_mutex_unlock(&taken_mutex);

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

  pthread_mutex_lock(&taken_mutex);
  takenSeats++;
  pthread_mutex_unlock(&taken_mutex);

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

  pthread_mutex_lock(&taken_mutex);
  takenSeats--;
  pthread_mutex_unlock(&taken_mutex);
}



void *booth(void *max_seats)
{
  char curr_request[WIDTH_REQUEST], feedback[WIDTH_FEEDBACK], *token;
  char pid[WIDTH_PID + 1], ansFIFO[3 + WIDTH_PID + 1];

  int pref_seats[MAX_CLI_SEATS], reserved_seats[MAX_CLI_SEATS];

  int num_wanted_seats, num_prefered_seats, num_room_seats = *(int *)max_seats;
  int errcode = 0;

  while (1)
  {
      // Get request from buffer
      pthread_mutex_lock(&req_mutex);
      while (request[0] == '\0')
        pthread_cond_wait(&full_req_cond, &req_mutex);

      strcpy(curr_request, request);
      request[0] = '\0';
      pthread_cond_signal(&empty_req_cond);
      pthread_mutex_unlock(&req_mutex);

      #ifdef DEBUG
        printf("Thread Got Request: %s\n", curr_request);
      #endif

      // Parse request
      token = strtok(curr_request, " ");  // Client PID
      strcpy(pid, token);
      token = strtok(NULL, " ");          // num_wanted_seats
      num_wanted_seats = atoi(token);

      if (atoi(pid) == -1)    return NULL; // Flag for termination

      #ifdef DEBUG
        printf("PID: %s N: %d\n", pid, num_wanted_seats);
      #endif

      token = strtok(NULL, " ");
      for (num_prefered_seats = 0; token != NULL; num_prefered_seats++)
      {
        pref_seats[num_prefered_seats] = atoi(token);
        token = strtok(NULL, " ");

        #ifdef DEBUG
          printf("Seat[%d]: %d\n", num_prefered_seats, pref_seats[num_prefered_seats]);
        #endif
      }

      #ifdef DEBUG
        printf("NP: %d\n", num_prefered_seats);
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

      if (getTakenSeats() >= MAX_ROOM_SEATS)
          SET_ERRCODE(errcode, -6);

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
        if (k < num_wanted_seats)
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
    #ifdef DEBUG
      printf("Created Booth\n");
    #endif
  }

  // Create Server fifo for receiving client requests
  if (mkfifo("requests", 0600) != 0)
  {
    perror("requests");
    exit(1);
  }

  // Open Server FIFO
  int requestfd = open("requests", O_RDONLY | O_NONBLOCK);
  if (requestfd < 0)
  {
    perror("Server: requests");
    exit(3);
  }

  // Read upcomming requests and put in buffer
  request[0] = '\0';

  while(difftime(time(NULL), initTime) < openTime)
  {
    char tmp_request[WIDTH_REQUEST];
    int r = 0;
    r = read(requestfd, tmp_request, WIDTH_REQUEST);

    if (r <= 0)   continue;

    pthread_mutex_lock(&req_mutex);
    while (request[0] != '\0')
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

  #ifdef DEBUG
    printf("Starting Thread termination %d\n", numBooths);
  #endif

  // Signal Threads to exit
  char term[WIDTH_PID + WIDTH_SEAT + 2];
  sprintf(term, "%0*d %0*d", WIDTH_PID, -1, WIDTH_SEAT, 0);

  for (int i = 0; i < numBooths; i++)
  {
    pthread_mutex_lock(&req_mutex);
    while (request[0] != '\0')
      pthread_cond_wait(&empty_req_cond, &req_mutex);

    strcpy(request, term);
    pthread_cond_signal(&full_req_cond);
    pthread_mutex_unlock(&req_mutex);
  }

  // Wait for threads to terminate
  for (int i = 0; i < numBooths; i++)
  {
    if (pthread_join(booths[i], NULL) != 0)
    {
      perror("Thread Join");
      exit(5);
    }
  }

  #ifdef DEBUG
    printf("Writing to files \n");
  #endif


  // Open book file
  int bookfd = open ("sbook.txt", O_WRONLY | O_CREAT, 0600);
  if (bookfd < 0)
  {
    perror("cbook.txt");
    exit(6);
  }

  // Insert data (Only thread by now so no need for sync)
  for (int i = 0; i < numSeats; i++)
  {
    if (seats.position[i] > 0)
    {
      char seat[WIDTH_SEAT + 2];
      sprintf(seat, "%0*d\n", WIDTH_SEAT, i);
      write(bookfd, seat, strlen(seat));
    }
  }

  // Close book file
  if (close(bookfd) < 0)
  {
    perror("cbook.txt");
    exit(7);
  }

  #ifdef DEBUG
    printf("Destroying structures\n");
  #endif

  // Destroy Synchronization Structures
  pthread_mutex_destroy(&req_mutex);
  pthread_cond_destroy(&full_req_cond);
  pthread_cond_destroy(&empty_req_cond);

  for (int i = 0; i < numSeats; i++)
    pthread_mutex_destroy(&(seats.mutex[i]));

  exit(0);
}
/*
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
*/
