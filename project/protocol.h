#ifndef PROTOCOL_H
#define PROTOCOL_H


#define MAX_CLI_SEATS   99  // Max num of seats that each client can reserve
#define WIDTH_PID       5
#define WIDTH_XXNN      5
#define WIDTH_SEAT      4

/*
    Protocol Format:
    Client -> Server:
      String with the first WIDTH_PID chars as PID,
      followed by the num of seats (arbitary length),
      followed by a sequence of SEATS with WIDTH_SEAT chars each
      followed by the seats themselves

      Ex: 02345 2 0456 0056 0057 0058 0059
          Client with PID 2345 wants 2 seats, from the 5 specified seats
 */

#endif
