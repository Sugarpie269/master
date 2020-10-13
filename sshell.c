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
#define MAX_PATH 32

enum PARSING_ERRORS{
        TOO_MANY_ARGS = -1,
        MISSING_COMMAND = -2,
        NO_OUTPUT_FILE = -3,
        CANNOT_OPEN_OUTPUT_FILE = -4,
        MISCLOCATED_OUTPUT_REDIRECTION = -5,
}; 

struct command{
        char *args[MAX_ARGS];
        int numberOfArguments;
};

struct CommandLine{
        struct command array_commands[MAX_ARGS];
        bool isRedirect;
        bool isPipe;
        int numberOfCommands;
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


void Redirection(const struct CommandLine structCmd){
        if(structCmd.array_commands!=NULL){
                printf("Not Empty\n");
        }
}


int main(void)
{
        char cmd[CMDLINE_MAX];
        char cmd_original[CMDLINE_MAX];
        char cmd_pl_Copy[CMDLINE_MAX];
        char cmd_rd_Copy[CMDLINE_MAX];
        char cwd[MAX_PATH];
        char *argv[MAX_ARGS];
        char *argPL[MAX_ARGS];
        char *argRD[MAX_ARGS];
        bool exit_bool = false;
        


        while (exit_bool == false) {
                char *nl;
                int status;
                pid_t pid;
                struct CommandLine structCmd;

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
                /*Make 3 versions of cmd for parsing*/
                memcpy(cmd_pl_Copy, cmd, sizeof(cmd));
                memcpy(cmd_rd_Copy, cmd, sizeof(cmd));

                /*Remove trailing newline from command line*/
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                nl = strchr(cmd_pl_Copy, '\n');
                if (nl)
                        *nl = '\0';

                nl = strchr(cmd_rd_Copy, '\n');
                if (nl)
                        *nl = '\0';

                /*Set all the value of arguments to NULL*/        
                memset(argv, '\0', sizeof(argv));
                memset(argPL, '\0', sizeof(argPL));
                memset(argRD, '\0', sizeof(argRD));
                
                /*Get the characters into an array words*/
                int value = ConvertToWords(cmd, argv);
                int hasPipe = ParsePL(cmd_pl_Copy, argPL) + 1;
                int hasRedir = ParseRD(cmd_rd_Copy, argRD) + 1;

                printf("value = %d, PL = %d, RD = %d \n", value, hasPipe, hasRedir);

                if (hasPipe == 0 && hasRedir == 0) {
                        structCmd.isPipe = false;
                        structCmd.isRedirect = false;
                        structCmd.numberOfCommands = 1;
                        
                        if (value == TOO_MANY_ARGS + 1) {
                                fprintf(stderr, "Error: too many process arguments");
                        }
                        else {
                                structCmd.array_commands[0].numberOfArguments = value;
                        }
                        CopyCharArray(structCmd.array_commands[0].args, argv, structCmd.array_commands[0].numberOfArguments);

                        /* Builtin command */
                        if (!strcmp(structCmd.array_commands[0].args[0], "exit")) {
                                fprintf(stderr, "Bye...\n");
                                exit_bool = true;
                                //Execute a command which implements the exit command 
                                break;
                        }
                        if (!strcmp(structCmd.array_commands[0].args[0], "cd")) {
                                //TODO: Error checking with changing directories
                                int eNotDir;
                                getcwd(cwd, sizeof(cwd));
                                //printf("Change CWD from '%s' to '%s'\n", cwd, argsWithoutNull[1]);
                                eNotDir = chdir(structCmd.array_commands[0].args[1]);
                                if (eNotDir == -1) {
                                        fprintf(stderr, "Error: No such directory.\n");
                                        //TODO: Figure dif between cannot cd into vs no such directory
                                }
                                continue;
                        }
                        if (!strcmp(structCmd.array_commands[0].args[0], "pwd")) {
                                getcwd(cwd, sizeof(cwd));
                                printf("%s\n", cwd);
                                continue;
                        /* Regular command */
                        for (int i = 0; i < structCmd.array_commands[0].numberOfArguments; i++) {
                                printf("| %s | ", structCmd.array_commands[0].args[i]);
                        }
                        printf("\n");

                        pid = fork();
                        if (pid == 0) {
                                /* Child Process*/
                                execvp(structCmd.array_commands[0].args[0], &structCmd.array_commands[0].args[0]);
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
                        structCmd.isRedirect = true;
                        structCmd.numberOfCommands = hasRedir - 1;
                        char* redirArgsTrim[structCmd.numberOfCommands]; //Should be size = 2
                        char* argvCommandsRedirect[MAX_ARGS];
                        CopyCharArray(redirArgsTrim, argRD, structCmd.numberOfCommands);
                        
                        for (int i = 0; i < structCmd.numberOfCommands; i++) {
                                structCmd.array_commands[i].numberOfArguments = ConvertToWords(redirArgsTrim[i], argvCommandsRedirect) + 1;
                                CopyCharArray(structCmd.array_commands[i].args, argvCommandsRedirect, structCmd.array_commands[i].numberOfArguments);
                                for(int j=0;j<structCmd.array_commands[i].numberOfArguments;j++){
                                        printf("{ %s } ", structCmd.array_commands[i].args[j]);
                                }
                        }

                        printf("%d", structCmd.numberOfCommands);

                        Redirection(structCmd);

                        printf("\n");
                }
                else if (hasPipe > 0 && hasRedir == 0) {
                        //If pipe:
                        //Implementation of piping
                        char* argvCommandsPipe[MAX_ARGS];
                        structCmd.numberOfCommands = hasPipe - 1;
                        char* pipeArgsTrim[structCmd.numberOfCommands];
                        CopyCharArray(pipeArgsTrim, argPL, structCmd.numberOfCommands);
                        for (int i = 0; i < structCmd.numberOfCommands; i++) {
                                structCmd.array_commands[i].numberOfArguments = ConvertToWords(pipeArgsTrim[i], argvCommandsPipe) + 1;
                                CopyCharArray(structCmd.array_commands[i].args, argvCommandsPipe, structCmd.array_commands[i].numberOfArguments);
                                for(int j=0;j<structCmd.array_commands[i].numberOfArguments;j++){
                                        printf("{ %s } ", structCmd.array_commands[i].args[j]);
                                }
                        }

                        //divide those commands to struct
                        printf("%d", structCmd.numberOfCommands);
                        printf("\n");
                }
                else {
                        //If both:
                        //After checking only 1 redir and less than 3 pipes
                        int sizeOfPipe = hasPipe;
                        int sizeOfRedir = hasRedir;
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
