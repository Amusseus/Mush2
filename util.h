#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* prints the shell left side */
void printAsk(FILE *c);

/* move onto nextline */
void moveOn();

void closeAllPipes(int ** pipefd, int len);