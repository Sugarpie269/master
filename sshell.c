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

int ConvertToWords_2(char cmd[], char *argv[]){
        struct CommandLine structCmd;
        int j=0;
        const char delim[] = " >|";
        char *token;
        structCmd.numPipeLine = 0;
        int numberOfArgs = 0;
        for(int i=0;i<=(int)strlen(cmd); i++){
                if(numberOfArgs <= 16){
                        if(cmd[i] != " "){
                                if(cmd[i] == ">"){
                                        argv[j] = ">";
                                        structCmd.args[j] = ">";
                                        structCmd.isRedirect = true;
                                        j++;
                                }else if(cmd[i] == "|"){
                                        argv[j] = "|";
                                        structCmd.args[j] = "|";
                                        structCmd.isPipe = true;
                                        structCmd.numPipeLine++;
                                        j++;
                                }else{
                                        strncat(token, &cmd[i], sizeof(token) + sizeof(cmd[i]));
                                }
                        }
                        argv[j] = token;
                        structCmd.args[j] = token;
                        token = NULL;
                        numberOfArgs++;
                        j++;
                }else{
                        return TOO_MANY_ARGS;
                }
                printf("%s , ", structCmd.args[j]);
        }
        return j;
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
        char *argv[MAX_ARGS];

        while (1) {
                char *nl;
                int status, sizeOfArgv;
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
                
                /*Get the characters into an array words*/        
                memset(argv, '\0', sizeof(argv));
                
                int value = ConvertToWords(cmd, argv) + 1;
                if(value == TOO_MANY_ARGS){
                        fprintf(stderr, "Error: too many process arguments");
                }else{
                        sizeOfArgv = value;
                }
                char *argsWithoutNull[sizeOfArgv];
                CopyCharArray(argsWithoutNull, argv, sizeOfArgv);


                /* Builtin command */
                if (!strcmp(argsWithoutNull[0], "exit")) {
                        fprintf(stderr, "Bye...\n");
                        //Execute a command which implements the exit command 
                        break;
                }

                /* Regular command */
                pid = fork();
                if(pid == 0){
                        /* Child Process*/
                        for(int i=0;i<sizeOfArgv;i++){
                                printf("| %s | ", argsWithoutNull[i]);
                        }
                        printf("%ld", sizeof(argsWithoutNull)/sizeof(char));
                        execvp(argsWithoutNull[0],&argsWithoutNull[0]);
                        perror("evecvp error in child");
                } else if(pid > 0){
                        /* Parent Process*/
                        waitpid(pid == P_PID, &status, 0);
                        cmd_original[strlen(cmd_original)-1]='\0';
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd_original, status);
                } else {
                        perror("fork");
                        exit(1);
                }
                
        }

        return EXIT_SUCCESS;
}
