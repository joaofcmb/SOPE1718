#ifndef simgrep_h
#define simgrep_h

typedef struct
{
  int ignoreCase;     // -i
  int showFileOnly;   // -l
  int showLineNum;    // -n
  int countLines;     // -c
  int wholeWordOnly;  // -w
  int recursive;       // -r
} options_t;

options_t setupOptions(char *argv[]);

#endif