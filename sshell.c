#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <stdbool.h>

#define CMDLINE_MAX 512
#define MAX_ARGS 100
#define MAX_PIPE_LINE 3
#define CMD_MAX_ARGS 17

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

struct DelimCommand{
        char delim;
        bool isCharBefore;
};

void mergeDelimsArgv(char *cmd[], char* argv[], struct DelimCommand delimCmd[], int sizeOfArgv, int sizeOfDelims){
        printf("Begin: %s\n", __func__);
        int j=0,k=0;
        int size = sizeOfArgv+sizeOfDelims;
        printf("size of delims = %d, size of argv = %d\n", sizeOfDelims, sizeOfArgv);
        for(int i=0;i<sizeOfArgv+sizeOfDelims;i++){
                printf("loop i = %d\n", i);
                if(k<sizeOfDelims && delimCmd[j].isCharBefore == false){
                        printf("If one\n");
                        cmd[i] = &delimCmd[j].delim;
                        printf("%s, ", cmd[i]);
                        j++;
                } else{
                        printf("Else\n");
                        if(k<sizeOfArgv) {
                                printf("Argc\n");
                                cmd[i] = argv[k];
                                k++;
                        }
                        if(j<sizeOfDelims) {
                                printf("\nDelim\n");
                                i = i+1;
                                cmd[i] = &delimCmd[j].delim;
                                j++;
                        }
                }
        }
        printf("\n");
        printf("Begin: Printing cmd\n");
        for(int j=0; j<size;j++){
                printf("%s, ", cmd[j]);
        }
        printf("\n");
        printf("End: Printing cmd\n");
        printf("End: %s\n", __func__);
}
/*int ConvertToWords(struct CommandLine *structCmd, char cmd[], char *argv[], int *totalArgumentsInclDelims){
        printf("Begin: %s\n", __func__);
        struct DelimCommand delimCmd[CMD_MAX_ARGS];
        int i=0, j=0, k=0, l=0;
        const char delim[] = " >|";
        char *token;
        bool isPrevChar = false;
        structCmd->numPipeLine = 0;

        if(cmd[0]!= delim[0] && cmd[0]!=delim[1] && cmd[0]!=delim[2]){
                isPrevChar = true;
                printf("cmd first char isAlpha\n");
        }

        for(j=0;j<(int)strlen(cmd);j++){
                if(cmd[j] == delim[1]){
                        delimCmd[k].isCharBefore = isPrevChar;
                        delimCmd[k].delim = delim[1];
                        structCmd->isRedirect = true;
                        k++;
                        isPrevChar = false;
                } else if(cmd[j] == delim[2]){
                        delimCmd[k].isCharBefore = isPrevChar;
                        delimCmd[k].delim = delim[2];
                        structCmd->isPipe = true;
                        structCmd->numPipeLine++;
                        k++;
                        isPrevChar = false;
                } else if(cmd[0]!= delim[0] && cmd[0]!=delim[1] && cmd[0]!=delim[2]){
                        isPrevChar = true;
                }     
                printf("\n%c", delimCmd[k-1].delim);
        }

        token = strtok(cmd, delim);
        while(token!=NULL && strcmp(token,"\n") != 0){
                if(i > 16){
                        return TOO_MANY_ARGS;
                }
                if(i == 0){
                        structCmd->command = token;
                }
                argv[i] = token;
                //argv[i+1] = &argDelims[k++]; 
                printf("argv[%d] = %s delim = %c, ", i, argv[i],  delimCmd[l].delim);
                token = strtok(NULL, delim);
                i = i + 1;
                l++;
        }
        *totalArgumentsInclDelims = i+k;
        char *return_cmd[*totalArgumentsInclDelims];
        mergeDelimsArgv(return_cmd, argv, delimCmd, i, k);
        structCmd->args = return_cmd;
        printf("Begin: Printing structCmd\n");
        for(int j=0; j<i+k;j++){
                printf("%s, ", structCmd->args[j]);
        }
        printf("\n");
        printf("End: Printing structCmd\n");
        printf("End: %s\n", __func__);
        return i;
}*/

/*void CopyCharArray(char *argsWithoutNull[], char *argv[], int sizeOfArgv){
        for(int i=0;i<sizeOfArgv;i++){
                argsWithoutNull[i] = argv[i];
                //printf("%s", argsWithoutNull[i]);
        }
        argsWithoutNull[sizeOfArgv] = NULL;
}*/

int main(void)
{
        char cmd[CMDLINE_MAX];
        char cmd_original[CMDLINE_MAX];
        char *argv[MAX_ARGS];
        bool bool_exit = false;

        while (1 && bool_exit == false) {
                char *nl;
                int status;
                pid_t pid;
                struct CommandLine structCmd;
                int totalArgumentsInclDelims = 0;
                int numberOfArguments = 0;

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
                numberOfArguments = ConvertToWords(&structCmd, cmd, argv, &totalArgumentsInclDelims);
                if(numberOfArguments == TOO_MANY_ARGS){
                        fprintf(stderr, "Error: too many process arguments");
                        break;
                }

                printf("size of struct cmd = %ld\n", sizeof(structCmd)/sizeof(char));
                /* Builtin command */
                if (!strcmp(structCmd.args[0], "exit")) {
                        fprintf(stderr, "Bye...\n");
                        bool_exit = true;
                        //Execute a command which implements the exit command 
                        break;
                }

                /* Regular command */
                pid = fork();
                if(pid == 0){
                        /* Child Process*/
                        for(int i=0;i<totalArgumentsInclDelims;i++){
                                printf("| %s | ", structCmd.args[i]);
                        }
                        printf("%ld\n", sizeof(structCmd.args)/sizeof(char));
                        execvp(structCmd.command,&structCmd.args[0]);
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
