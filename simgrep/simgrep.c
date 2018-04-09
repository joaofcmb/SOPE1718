#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

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

  exit(fileSearch(&options, argv[options.pathI]));
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

int fileSearch(options_t *options, char *dirPath)
{
  DIR *dir;
  struct dirent *file;
  struct stat fileInfo;
  if ((dir = opendir(dirPath)) == NULL)
  {
    perror(dirPath);
    return -1;
  }

  printf("parsing directory of path \"%s\"\n", dirPath);

  if (options->recursive)
  { // find the directories within this directory and search through them recursively
    while ((file = readdir(dir)) != NULL)
    {
      // check if is dir and call fileSearch in a new process
      if (stat(strcat(file->d_name, dirPath), &fileInfo) != 0)
      {
        perror(file->d_name);
        continue;
      }

      if (S_ISDIR(fileInfo.st_mode))
      {
        #ifdef DEBUG
        printf("found directory \"%s\", starting new process.\n", file->d_name);
        #endif

        //TODO start new process to execute fileSearch
      }
    }
    rewinddir(dir);
  }

  while ((file = readdir(dir)) != NULL)
  {
      // check if is regular file and process it
      if (stat(strcat(file->d_name, dirPath), &fileInfo) != 0)
      {
        perror(file->d_name);
        continue;
      }

      if (S_ISREG(fileInfo.st_mode))
      {
        #ifdef DEBUG
        printf("found regular file \"%s\", parsing file for pattern\n", file->d_name);
        #endif
      }

      //TODO process file
  }

  return 0;
}
