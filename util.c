#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define WRITE_END 1
#define READ_END 0

void printAsk(FILE *c){
    /* print and flush */
    fprintf(c,"8-P ");
    /* check for flush error */
    if(-1 == fflush(c)){
        perror("fflsuh");
        exit(EXIT_FAILURE); 
    }
}

void moveOn(){
    printf("\n");
}

void closeAllPipes(int **pipefd, int len){
    int i;
    for(i = 0; i < len ; i++){
        close(pipefd[i][READ_END]);
        close(pipefd[i][WRITE_END]);
    }
}