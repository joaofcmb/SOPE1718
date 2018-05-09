#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string.h>

#define MAX_ROOM_SEATS  9999  // Max num of seats in the room to be reserved
#define MAX_CLI_SEATS   99    // Max num of seats that each client can reserve
#define WIDTH_SEATNO    2     // Width of num_wanted_seats (Must be synced with MAX_CLI_SEATS)
#define WIDTH_PID       5     
#define WIDTH_XXNN      5
#define WIDTH_SEAT      4

/*
    Protocol Format:
    Client -> Server:
      first element is the client PID,
      followed by the num of seats,
      followed by a sequence of prefered seats

      Ex: 02345 0002 0456 0056 0057 0058 0059

          Client with PID 2345 wants 2 seats, from the 5 specified seats
 */
 #define WIDTH_REQUEST  WIDTH_PID + (MAX_ROOM_SEATS + 1) * (WIDTH_SEAT + 1) + 1

#endif
