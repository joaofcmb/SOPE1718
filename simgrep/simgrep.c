#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>
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

  int ret = fileSearch(&options, argv[options.pathI], argv[options.patternI]);

  while (wait(NULL) > 0);

  exit(ret);
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

int fileSearch(options_t *options, char *dirPath, char *pattern)
{
  DIR *dir;
  struct dirent *file;
  struct stat fileInfo;
  char filePath[257];

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
      if (sprintf(filePath, "%s/%s", dirPath, file->d_name) < 0)
      {
        perror(file->d_name);
        continue;
      }
      if (stat(filePath, &fileInfo) != 0)
      {
        perror(filePath);
        continue;
      }

      if (S_ISDIR(fileInfo.st_mode) &&
      strncmp(file->d_name, "..", 2) && strncmp(file->d_name, ".", 2))
      {
        #ifdef DEBUG
        printf("found sub-directory \"%s\", starting new process.\n", filePath);
        #endif

        // start new process to execute fileSearch
        pid_t pid;
        if ((pid = fork()) < 0)
        {
          perror(file->d_name);
          continue;
        }

        if (pid == 0)
        {
          int ret = fileSearch(options, filePath, pattern);

          while (wait(NULL) > 0);

          exit(ret);
        }
      }
    }
    rewinddir(dir);
  }

  while ((file = readdir(dir)) != NULL)
  {
      // check if is regular file and process it
      if (sprintf(filePath, "%s/%s", dirPath, file->d_name) < 0)
      {
        perror(file->d_name);
        continue;
      }
      if (stat(filePath, &fileInfo) != 0)
      {
        perror(filePath);
        continue;
      }

      if (S_ISREG(fileInfo.st_mode))
      {
        #ifdef DEBUG
        printf("found regular file \"%s\", parsing file for pattern\n", filePath);
        #endif
      }

      programa(pattern, filePath, options);
  }  
  return 0;
}

void programa(char* pat, char* fich, options_t *options)
{
  char const* const fileName = fich; /* should check that argc > 1 */
  FILE* file = fopen(fileName, "r"); /* should check the result */
  char line[256];
  int count = 0;
  int countl = 0;

  while (fgets(line, sizeof(line), file))
  {
    if(strstr(line, pat) != NULL)
    {
      countl++;

      if(options->ignoreCase == 1){
        gnoreCase(line); // -i
      }
      if(options->showFileOnly == 1){
        showFileOnly(fich);  // -l
      }
      if(options->showLineNum == 1){
        showLineNum(count); // -n
      }
      if(options->countLines == 1){
        countLines(countl); // -c
      }
      if(options->wholeWordOnly == 1){
        wholeWordOnly(line, pat, fich); // -w
      }
      //printf("%s", line);

    }

    count++;

  }
   fclose(file);
}

void gnoreCase(char linha[256]) // -i
{
  printf("%s", linha);
}

void showLineNum(int c) // -n
{
  printf("%d", c);
}

void showFileOnly(char* f) // -l
{
  printf("%s", f);
}

void countLines(int ls) // -c
{
  printf("%d", ls);
}

void wholeWordOnly(char linha[256], char* p, char* f)
{
  char * spaceP = NULL;
  char * spacePspace = NULL;
  char * Pspace = NULL;

  asprintf(&spaceP, "%s%s", " ", p);
  asprintf(&spacePspace, "%s%s%s", " ", p, " ");
  asprintf(&Pspace, "%s%s", p, " ");



  // size_t len = strlen(linha);
  // // char * line2[256];
  // char * line2 = malloc(len + 1 + 1);
  // strcpy(line2, linha);
  // //line2[len] = 'P';
  // line2[len] = 'P';
  //line2[len + 1] = '\0';
  //str_append(str,c)
  // char p0[256] = &p + " ";
  // char p1[256] = " " + &p;
  // char p2[256] = " " + &p + " ";

  // printf("%s", line2);

  // if(strstr(linha, p0) || strstr(linha, p1) || strstr(linha, p2))
  // {
  //   printf("%s", linha);
  // }

}
