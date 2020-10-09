#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h> 

#define CMDLINE_MAX 512
#define MAX_ARGS 16

void ConvertToWords(char cmd[], char *argv[]){
        int i=0;
        const char delim[2] = " ";
        char *token;
        token = strtok(cmd, delim);
        while(token!=NULL && strcmp(token,"\n") != 0){
                argv[i] = token;
                token = strtok(NULL, delim);
                i++;
        }
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char cmd_original[CMDLINE_MAX];
        char *argv[MAX_ARGS];

        while (1) {
                char *nl;
                int retval = 0;
                int status;

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

                /*Get the characters into an array words*/
                memset(argv, '\0', sizeof(argv));
                memcpy(cmd_original, cmd, sizeof(cmd));
                ConvertToWords(cmd, argv);
                
                // ask in the discussion printf("blah %s dsfs", cmd);
                //Remove trailing newline from command line
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(argv[0], "exit")) {
                        fprintf(stderr, "Bye...\n");
                        //Execute a command which implements the exit command 
                        break;
                }

                /* Regular command */
                if(!fork()){
                        retval = execvp(argv[0], argv);
                } else{
                        waitpid(-1, &status, 0);
                        cmd_original[strlen(cmd_original)-1]='\0';
                        printf("+ completed '%s' [%d]\n", cmd_original, retval);
                        retval = 0;
                }
                
        }

        return EXIT_SUCCESS;
}
