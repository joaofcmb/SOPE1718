#include <stdio.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
  pid_t pid = getpid();

  // Create client fifo for server feedback
  char[] ansFIFO = sprintf("ans%d", pid);
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

  // TODO Normalize arguments and write on requests fifo


  exit(0);
}
