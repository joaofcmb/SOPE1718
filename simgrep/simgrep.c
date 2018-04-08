#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "simgrep.h"

int main(int argc, char *argv[], char* envp[])
{
  options_t options = setupOptions(argv);

  #ifdef DEBUG
  printf("## simgrep init ##\n");
  printf("options struct values:|%d|%d|%d|%d|%d|%d|%d|%d|\n",
          options.ignoreCase, options.showFileOnly,
          options.showLineNum, options.countLines,
          options.wholeWordOnly, options.recursive,
          options.patternI, options.pathI);
  printf("## start of file search ##\n");
  #endif

  exit(fileSearch(&options, argv));
}


options_t setupOptions(char *argv[])
{
  options_t options = {0, 0, 0, 0, 0, 0, 0, 0};

  for (int i = 1; argv[i] != NULL; i++)
  {
    if      (!strncmp(argv[i], "-i", 2))  options.ignoreCase = 1;
    else if (!strncmp(argv[i], "-l", 2))  options.showFileOnly = 1;
    else if (!strncmp(argv[i], "-n", 2))  options.showLineNum = 1;
    else if (!strncmp(argv[i], "-c", 2))  options.countLines = 1;
    else if (!strncmp(argv[i], "-w", 2))  options.wholeWordOnly = 1;
    else if (!strncmp(argv[i], "-r", 2))  options.recursive = 1;

    else if (options.patternI == 0)       options.patternI = i;
    else                                  options.pathI = i;
  }

  return options;
}

int fileSearch(options_t *options, char *argv[])
{
  DIR *dir;
  struct dirent *file;

  if ((dir = opendir(argv[options->pathI])) == NULL)
  {
    perror(argv[options->pathI]);
    return -1;
  }

  if (options->recursive)
  { // find the directories within this directory and search through them recursively
    while ((file = readdir(dir)) != NULL)
    {
        // check if is dir and call fileSearch in a new process
    }
    rewinddir(dir);
  }

  while ((file = readdir(dir)) != NULL)
  {
      // check if is regular file and process it
  }

  return 0;
}
