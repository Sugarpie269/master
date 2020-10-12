#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <stdbool.h>

#define CMDLINE_MAX 512
#define MAX_ARGS 17
#define MAX_PIPE_LINE 3

enum PARSING_ERRORS{
        TOO_MANY_ARGS = -1,
        MISSING_COMMAND = -2,
        NO_OUTPUT_FILE = -3,
        CANNOT_OPEN_OUTPUT_FILE = -4,
        MISCLOCATED_OUTPUT_REDIRECTION = -5,
}; 

struct CommandLine{
        char **args;
        char *command;
        bool isRedirect;
        bool isPipe;
        int numPipeLine;
};

int ConvertToWords(char cmd[], char *argv[]){
        int i=0;
        const char delim[] = " >|";
        char *token;
        token = strtok(cmd, delim);
        while(token!=NULL && strcmp(token,"\n") != 0){
                if(i > 16){
                        return TOO_MANY_ARGS;
                }
                argv[i] = token;
                printf("argv[%d] = %s , ", i, argv[i]);
                token = strtok(NULL, delim);
                i++;
        }
        printf("\n");
        return i;
}

int ParseRD(char cmd[], char* argv[]) {
        int i = 0;
        const char delim[] = ">";
        char* token;
        token = strtok(cmd, delim);
        while (token != NULL && strcmp(token, "\n") != 0) {
                if (i > 16) {
                        return TOO_MANY_ARGS;
                }
                argv[i] = token;
                printf("RD_argv[%d] = %s , ", i, argv[i]);
                token = strtok(NULL, delim);
                i++;
        }
        if (i == 1) {
                printf("\n No Redirect.\n");
                return -1;
        }
        printf("\n");
        return i;
}

int ParsePL(char cmd[], char* argv[]) {
        int i = 0;
        const char delim[] = "|";
        char* token;
        token = strtok(cmd, delim);
        while (token != NULL && strcmp(token, "\n") != 0) {
                if (i > 16) {
                        return TOO_MANY_ARGS;
                }
                argv[i] = token;
                printf("PL_argv[%d] = %s , ", i, argv[i]);
                token = strtok(NULL, delim);
                i++;
        }
        if (i == 1) {
                printf("\n No Pipe.\n");
                return -1;
        }
        printf("\n");
        return i;
}

void CopyCharArray(char *argsWithoutNull[], char *argv[], int sizeOfArgv){
        for(int i=0;i<sizeOfArgv;i++){
                argsWithoutNull[i] = argv[i];
                //printf("%s", argsWithoutNull[i]);
        }
        argsWithoutNull[sizeOfArgv] = NULL;
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char cmd_original[CMDLINE_MAX];
        char cmd_pl_Copy[CMDLINE_MAX];
        char cmd_rd_Copy[CMDLINE_MAX];
        char *argv[MAX_ARGS];
        char *argPL[MAX_ARGS];
        char *argRD[MAX_ARGS];
        


        while (1) {
                char *nl;
                int status, sizeOfArgv;
                int sizeOfRedir, sizeOfPipe;
                pid_t pid;

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /*Copy cmd to a new cmd*/
                memcpy(cmd_original, cmd, sizeof(cmd));


                /*Remove trailing newline from command line*/
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /*Make 3 versions of cmd for parsing*/
                memcpy(cmd_pl_Copy, cmd, sizeof(cmd));
                memcpy(cmd_rd_Copy, cmd, sizeof(cmd));

                /*Get the characters into an array words*/        
                memset(argv, '\0', sizeof(argv));
                memset(argPL, '\0', sizeof(argPL));
                memset(argRD, '\0', sizeof(argRD));
                
                int value = ConvertToWords(cmd, argv) + 1;
                int hasPipe = ParsePL(cmd_pl_Copy, argPL) + 1;
                int hasRedir = ParseRD(cmd_rd_Copy, argRD) + 1;

                printf("value = %d, PL = %d, RD = %d \n", value, hasPipe, hasRedir);

                if (hasPipe == 0 && hasRedir == 0) {
                        
                        if (value == TOO_MANY_ARGS + 1) {
                                fprintf(stderr, "Error: too many process arguments");
                        }
                        else {
                                sizeOfArgv = value;
                        }
                        char* argsWithoutNull[sizeOfArgv];
                        CopyCharArray(argsWithoutNull, argv, sizeOfArgv);

                        /* Builtin command */
                        if (!strcmp(argsWithoutNull[0], "exit")) {
                                fprintf(stderr, "Bye...\n");
                                //Execute a command which implements the exit command 
                                break;
                        }
                        /* Regular command */
                        for (int i = 0; i < sizeOfArgv; i++) {
                                printf("| %s | ", argsWithoutNull[i]);
                        }
                        printf("%ld", sizeof(argsWithoutNull) / sizeof(char));
                        printf("\n");

                        pid = fork();
                        if (pid == 0) {
                                /* Child Process*/
                                execvp(argsWithoutNull[0], &argsWithoutNull[0]);
                                perror("evecvp error in child");
                        }
                        else if (pid > 0) {
                                /* Parent Process*/
                                waitpid(pid == P_PID, &status, 0);
                                cmd_original[strlen(cmd_original) - 1] = '\0';
                                fprintf(stderr, "+ completed '%s' [%d]\n", cmd_original, status);
                        }
                        else {
                                perror("fork");
                                exit(1);
                        }
                }
                else if (hasPipe == 0 && hasRedir > 0) {
                        //If redir:
                        //TODO: Check if its redir appending
                        //Implementation of redir
                        sizeOfRedir = hasRedir;
                        char* redirArgsTrim[sizeOfRedir]; //Should be size = 2
                        CopyCharArray(redirArgsTrim, argRD, sizeOfRedir);
                        for (int i = 0; i < sizeOfRedir; i++) {
                                printf("[ %s ] ", redirArgsTrim[i]);
                        }
                        printf("%ld", sizeof(redirArgsTrim) / sizeof(char));
                        printf("\n");
                }
                else if (hasPipe > 0 && hasRedir == 0) {
                        //If pipe:
                        //Implementation of piping
                        sizeOfPipe = hasPipe;
                        char* pipeArgsTrim[sizeOfPipe];
                        CopyCharArray(pipeArgsTrim, argPL, sizeOfPipe);
                        for (int i = 0; i < sizeOfPipe; i++) {
                                printf("{ %s } ", pipeArgsTrim[i]);
                        }
                        printf("%ld", sizeof(pipeArgsTrim) / sizeof(char));
                        printf("\n");
                }
                else {
                        //If both:
                        //After checking only 1 redir and less than 3 pipes
                        sizeOfPipe = hasPipe;
                        sizeOfRedir = hasRedir;
                        char* pipeArgsTrim[sizeOfPipe];
                        char* redirArgsTrim[sizeOfRedir]; //Should be size = 2

                        CopyCharArray(pipeArgsTrim, argPL, sizeOfPipe);
                        CopyCharArray(redirArgsTrim, argRD, sizeOfRedir);

                        for (int i = 0; i < sizeOfRedir; i++) {
                                printf("[ %s ] ", redirArgsTrim[i]);
                        }
                        printf("%ld", sizeof(redirArgsTrim) / sizeof(char));
                        printf("\n");

                        for (int i = 0; i < sizeOfPipe; i++) {
                                printf("{ %s } ", pipeArgsTrim[i]);
                        }
                        printf("%ld", sizeof(pipeArgsTrim) / sizeof(char));
                        printf("\n");

                        //Conduct piping mechanism up until the last argument (asuming each piping action is correct)
                                //If any piping actions fail, the following will fail in some way...?

                        //Last element should be something like [action > file]
                        //Thus, parseRedir->last element, pipe into action and write to file
                                //If Redir fails, use errors that would raise in redir
                }

                
        }

        return EXIT_SUCCESS;
}
