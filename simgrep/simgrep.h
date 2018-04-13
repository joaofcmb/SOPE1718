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

  int patternI;
  int pathI;
} options_t;

options_t setupOptions(char *argv[]);

int fileSearch(options_t *options, char *dirPath, char *pattern);

void programa(char* pat, char* fich, options_t *options);

void gnoreCase(char linha[256]);
void showFileOnly(char* f);
void showLineNum(int c);
void countLines(int ls);
void wholeWordOnly(char linha[256], char* p, char* f);

#endif
