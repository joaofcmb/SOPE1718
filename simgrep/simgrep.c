#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "simgrep.h"

int main(int argc, char *argv[], char* envp[])
{
  options_t options = setupOptions(argv);

  #ifdef DEBUG
  printf("%d|%d|%d|%d|%d|%d\n", options.ignoreCase, options.showFileOnly,
                                options.showLineNum, options.countLines,
                                options.wholeWordOnly, options.recursive);
  #endif
  
  exit(0);
}


options_t setupOptions(char *argv[])
{
  options_t options = {0, 0, 0, 0, 0, 0};

  for (int i = 0; argv[i] != NULL; i++)
  {
    if      (!strncmp(argv[i], "-i", 2))  options.ignoreCase = 1;
    else if (!strncmp(argv[i], "-l", 2))  options.showFileOnly = 1;
    else if (!strncmp(argv[i], "-n", 2))  options.showLineNum = 1;
    else if (!strncmp(argv[i], "-c", 2))  options.countLines = 1;
    else if (!strncmp(argv[i], "-w", 2))  options.wholeWordOnly = 1;
    else if (!strncmp(argv[i], "-r", 2))  options.recursive = 1;
  }

  return options;
}
