#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string.h>

#define MAX_ROOM_SEATS  9999
#define MAX_CLI_SEATS   99  // Max num of seats that each client can reserve
#define WIDTH_PID       5
#define WIDTH_XXNN      5
#define WIDTH_SEAT      4

/*
    Protocol Format:
    Client -> Server:
      first element is the client PID,
      followed by the num of seats,
      followed by a sequence of prefered seats

      Ex: 02345 2 0456 0056 0057 0058 0059

          Client with PID 2345 wants 2 seats, from the 5 specified seats
 */

#endif
