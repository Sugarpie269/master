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
                printf("%d = ", i);
                printf("%s\n", token);
                token = strtok(NULL, delim);
                i++;
        }
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char *argv[MAX_ARGS];

        while (1) {
                char *nl;
                int retval;
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
                ConvertToWords(cmd, argv);

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        //Execute a command which implements the exit command 
                        break;
                }

                /* Regular command */
                if(!fork()){
                        //printf("argv[0] = %s\n, argv[1] = %s\n, argv[2] = %s\n", argv[0], argv[1], argv[2]);
                        retval = execvp(argv[0], argv);
                        fprintf(stdout, "Return status value for '%s': %d\n",
                        cmd, retval);
                } else{
                        waitpid(-1, &status, 0);
                }
                
        }

        return EXIT_SUCCESS;
}
