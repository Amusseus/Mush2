#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include "mush.h"
#include "util.h"
#define READ_END 0
#define WRITE_END 1
#define KEEPRUN 1

char shellMode = 0;

/* signal handler */
void handler(int num){
}

/* 0 args = Mode 0 = Interactive Mode
*  1 args = Mode 1 = Batch Mode    */

int main(int argc, char *argv[]){
    
    /* IN & OUT used my mush2 */
    FILE *IN = stdin;
    FILE *OUT = stdout;
    
    struct sigaction sa; /* sig handler */
    /* setUp SIGINT handler */
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(-1 == sigaction(SIGINT, &sa, NULL)){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* batch processing mode */
    if(argc > 1){
        /* replace stdin with given script */
        IN = fopen(argv[1],"r");
        /* if script doesn't exist, return to interactive mode */
        if(IN == NULL){
            IN = stdin; 
        }else{
            shellMode = 1;
        }
    }

    /* Run forever until User ends */
    while(KEEPRUN){
        char *inStr;        /* line input recieved by mush2 */
        struct pipeline *p; /* used to parse line*/

        /* get line input from User */
        if(!shellMode){
            printAsk(OUT);
        } 
        inStr = readLongString(IN);

        /* failure or EOF */
        if(inStr == NULL){
            /* if EOF and not error */
            if(feof(IN)){
                break; /* User end mush2 */
            }
            /* ERR -> newline and restart*/
            moveOn();
            continue;
        }

        /* Input not empty then parse, else reset */
        if(strlen(inStr)){
            p = crack_pipeline(inStr);
            /* crack_pipeline ERR */
            if(p == NULL){
                free(inStr);
                continue;
            }
        } else{
            continue;
        }
        
        /* print_pipeline(stdout,p); */

        /* User Input cd [DIR] */
        if(strcmp("cd",p->stage->argv[0]) == 0){
            /* if DIR option given */
            if(p->stage->argc > 1){
                /* change dir */
                if(-1 == chdir(p->stage->argv[1])){
                    /* chdir fails */
                    perror(p->stage->argv[1]);
                    free_pipeline(p);
                    free(inStr);
                    continue;
                }
            } else{
                /* no [DIR], cd to HOME */
                char *home;          /* home dir */
                struct passwd *user; /* user data -> use if home fails */
                home = getenv("HOME");
                /* getenv -> HOME fails */
                if(home == NULL){
                    user = getpwuid(getuid());
                    /* fails to get pwuid */
                    if(user == NULL){
                       fprintf(stderr,"unable to determine home directory\n");
                        free_pipeline(p);
                        free(inStr);
                        continue;
                    }
                    home = user->pw_dir;
                }
                /* change to home dir */
                if(-1 == chdir(home)){
                    perror(home);
                    free_pipeline(p);
                    free(inStr);
                    continue;
                }
            }

        } else{ 
            int i;                       /* variable for loop */
            int numChild = 0;            /* num of childs forked */
            pid_t child;                 /* id of child created */
            struct clstage curStg;       /* current stage on */
            char usePipe = 0;            /* PIPE NEEDED ? */
            char failPipe = 0;           /* has pipe() resulted in ERR */
            int status;
            int numPipes = p->length - 1;
            /* fd for all pipes **if needed** */
            int **pipefd = (int **)calloc(numPipes, sizeof(int *)); 
            if(pipefd == NULL){
                perror("calloc");
                break;
            }
            /* stage # > 1, USE pipe */
            if(p->length > 1){
                usePipe = 1;
            }

            /* generate all pipes| # pipes = # stages -1 */
            for(i = 0; i < numPipes ; i++){
                pipefd[i] = (int *)calloc(2, sizeof(int));
                if(pipefd[i] == NULL){
                    perror("calloc");
                    failPipe = 1;
                    break;
                }
                if(-1 == pipe(pipefd[i])){
                    /* pipe() failed */
                    perror("pipe");
                    failPipe = 1;
                    break;
                }
            }

            /* if any pipe() fails, GOTO next command */
            if(failPipe){
                continue;
            }

            /* fork each stage */
            for(i = 0; i < p->length ; i++){
                curStg = p->stage[i];
                if((child = fork())){
                    if(-1 == child){
                        perror("fork");
                        break;
                    }
                    /* if fork doesn't fail */
                    numChild++;
                } else{
                    /* IN CHILD */
                    /* input redirection */
                    if(curStg.inname != NULL){
                        int tempinfd;
                        if(-1 == (tempinfd = open(curStg.inname,O_RDONLY))){
                            perror("open");
                        }
                        if(-1 == dup2(tempinfd,STDIN_FILENO)){
                            perror("dup2");
                        }
                        close(tempinfd);
                    }
                    /* output redireaction */
                    if(curStg.outname != NULL){
                        int tempoutfd;
                        if(-1 == (tempoutfd = open(curStg.outname,O_WRONLY 
                        | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | 
                        S_IWGRP | S_IROTH | S_IWOTH))){
                            perror("open");
                        }
                        if(-1 == dup2(tempoutfd,STDOUT_FILENO)){
                            perror("dup2");
                        }
                        close(tempoutfd);
                    } 

                    if(usePipe){
                        if(i == 0){
                            /* first stage */
                            if(-1 == dup2(pipefd[i][1],STDOUT_FILENO)){
                                perror("dup2");
                                break;   
                            }
                        } else if(i == (p->length -1)){
                            /* last stage */
                            if(-1 == dup2(pipefd[i-1][0],STDIN_FILENO)){
                                perror("dup2");
                                break;   
                            }
                        } else{
                            /* middle stages */
                            if(-1 == dup2(pipefd[i-1][0],STDIN_FILENO)){
                                perror("dup2");
                                break;   
                            }
                            if(-1 == dup2(pipefd[i][1],STDOUT_FILENO)){
                                perror("dup2");
                                break;   
                            }
                        }
                        closeAllPipes(pipefd,numPipes);
                    }
                    /* runs the stage */
                    if(-1 == execvp(curStg.argv[0],curStg.argv)){
                        perror(curStg.argv[0]);
                        exit(EXIT_FAILURE);
                    } 
                }
            }

            closeAllPipes(pipefd,numPipes);
            /* free all pipes */
            for(i = 0; i < numPipes; i++){
                free(pipefd[i]);
            }
            free(pipefd);
            /* waiting for all childs to finish */
            while(numChild){
                /* wait for one process to end */
                if( -1 == wait(&status)){
			        perror("wait");
		        }
                /* reset if any of the programs failed */
                if(WEXITSTATUS(status)){
                    numChild = 0;
                    continue;
                }
                numChild--;
            }
        }      

        /* free pipeline and input */
        free_pipeline(p);
        free(inStr);
    }
    return 0;
}
