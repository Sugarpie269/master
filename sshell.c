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

enum PARSING_ERRORS
{
        TOO_MANY_ARGS = -1,
        MISSING_COMMAND = -2,
        NO_OUTPUT_FILE = -3,
        CANNOT_OPEN_OUTPUT_FILE = -4,
        MISCLOCATED_OUTPUT_REDIRECTION = -5,
};

struct CommandLine
{
        char **args;
        char *command;
        bool isRedirect;
        bool isPipe;
        int numPipeLine;
};

/*
int ConvertToWords(char cmd[], char *argv[])
{
        int i = 0;
        const char delim[] = " >|";
        char *token;
        token = strtok(cmd, delim);
        while (token != NULL && strcmp(token, "\n") != 0)
        {
                if (i > 16)
                {
                        return TOO_MANY_ARGS;
                }
                argv[i] = token;
                printf("argv[%d] = %s , ", i, argv[i]);
                token = strtok(NULL, delim);
                i++;
        }
        printf("\n");
        return i;
}*/

int GetTokens(char **argv, char cmd[], const char delim[])
{
        printf("Begin: %s\n", __func__);
        int i = 0;
        char *token;
        token = strtok(cmd, delim);
        printf("Begin: While loop\n");
        while (token != NULL && strcmp(token, "\n") != 0)
        {
                if (i > 16)
                {
                        return TOO_MANY_ARGS;
                }
                argv = (char **)realloc(argv, sizeof(argv) + sizeof(token));
                argv[i] = token;
                printf("argv[%d] = %s , ", i, argv[i]);
                token = strtok(NULL, delim);
                i++;
        }
        printf("End: While loop\n");
        printf("\n");
        printf("End: %s\n", __func__);
        return i;
}

int ConvertToWords(char cmd[], __attribute__((unused)) char **argv)
{
        printf("Begin: %s\n", __func__);
        const char delim_pipe[] = "|";
        const char delim_redirect[] = ">";
        char **argv_pipe = NULL, **argv_redirect = NULL, **argv_normal = NULL;
        int get_num_tokens_pipe = 0, get_num_tokens_redirect = 0, get_num_tokens_normal = 0;
        argv = NULL;
        printf("Normal\n");
        get_num_tokens_normal = GetTokens(argv_normal, cmd, delim_redirect);
        printf("Pipe\n");
        get_num_tokens_pipe = GetTokens(argv_pipe, cmd, delim_pipe);
        printf("Redirect\n");
        get_num_tokens_redirect = GetTokens(argv_redirect, cmd, delim_redirect);
        if (get_num_tokens_redirect > get_num_tokens_pipe)
        {
                argv = (char **)realloc(argv, sizeof(argv_redirect));
                argv = argv_redirect;
                printf("End: %s\n", __func__);
                return get_num_tokens_redirect;
        }
        else if (get_num_tokens_redirect < get_num_tokens_pipe)
        {
                argv = (char **)realloc(argv, sizeof(argv_pipe));
                argv = argv_pipe;
                printf("End: %s\n", __func__);
                return get_num_tokens_pipe;
        }
        else
        {
                argv = (char **)realloc(argv, sizeof(argv_pipe));
                argv = argv_pipe;
                printf("argv_pipe[0] = %s", argv_pipe[0]);
                printf("End: %s\n", __func__);
                return get_num_tokens_pipe;
        }
        printf("End: %s\n", __func__);
}

/*void CopyCharArray(char *argsWithoutNull[], char *argv[], int sizeOfArgv)
{
        printf("Begin: %s\n", __func__);
        for (int i = 0; i < sizeOfArgv; i++)
        {
                argsWithoutNull[i] = argv[i];
                printf("%s", argsWithoutNull[i]);
        }
        argsWithoutNull[sizeOfArgv] = NULL;
        printf("End: %s\n", __func__);
}*/

int main(void)
{
        char cmd[CMDLINE_MAX];
        char cmd_original[CMDLINE_MAX];
        bool exit_bool = false;

        while (1 && exit_bool == false)
        {
                char *nl;
                int status;
                char **argv = NULL;
                int numberOfArguments = 0;

                pid_t pid;

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO))
                {
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
                numberOfArguments = ConvertToWords(cmd, argv);
                if (numberOfArguments >= MAX_ARGS)
                {
                        fprintf(stderr, "Error: too many process arguments");
                }
                //char *argsWithoutNull[numberOfArguments];
                //CopyCharArray(argsWithoutNull, argv, numberOfArguments);
                printf("Argv = %s\n", argv[0]);
                /* Builtin command */
                if (!strcmp(argv[0], "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        exit_bool = true;//Execute a command which implements the exit command
                        break;
                }

                /* Regular command */
                pid = fork();
                if (pid == 0)
                {
                        /* Child Process*/
                        for (int i = 0; i < numberOfArguments; i++)
                        {
                                printf("| %s | ", argv[i]);
                        }
                        printf("%ld", sizeof(argv) / sizeof(char));
                        execvp(argv[0], &argv[0]);
                        perror("evecvp error in child");
                }
                else if (pid > 0)
                {
                        /* Parent Process*/
                        waitpid(pid == P_PID, &status, 0);
                        cmd_original[strlen(cmd_original) - 1] = '\0';
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd_original, status);
                }
                else
                {
                        perror("fork");
                        exit(1);
                }
        }

        return EXIT_SUCCESS;
}
