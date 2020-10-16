#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dirent.h>

#define CMDLINE_MAX 512
#define MAX_ARGS 17
#define MAX_PIPE_LINE 3
#define MAX_PATH 32

#define EXIT "exit"
#define CD "cd"
#define PWD "pwd"
#define SLS "sls"
#define delim_space " "
#define delim_pipe "|"
#define delim_redirect ">"
#define current "."

enum PARSING_ERRORS
{
        TOO_MANY_ARGS = -1,
        MISSING_COMMAND = -2,
        NO_OUTPUT_FILE = -3,
        CANNOT_OPEN_OUTPUT_FILE = -4,
        MISCLOCATED_OUTPUT_REDIRECTION = -5,
        NO_DIRECTORY = -6,
        EXIT_ERROR = -7,
        NO_ERROR = -8,
};

enum DELIMS
{
        SPACE = 0,
        PIPE = 1,
        REDIRECT = 2,
        NUMBER_OF_DELIMS = 3,
};

struct Command
{
        char *args[MAX_ARGS];
        int numberOfArguments;
};

enum STATUS
{
        SUCCESS = 0,
        FAILURE = 1,
};

struct CommandLine
{
        struct Command array_commands[MAX_ARGS];
        char cmd[CMDLINE_MAX];
        bool isRedirect;
        bool isPipe;
        int numberOfCommands;
};

int ConvertToWords(char cmd[], char *argv[], const char delim[])
{
        int i = 0;
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
        if (i == 1 && strcmp(delim, " ") != 0)
        {
                printf("\n No Redirect. and No Pipe\n");
                return -1;
        }
        printf("\n");
        return i;
}

void CopyCharArray(char *argsWithoutNull[], char *argv[], int sizeOfArgv)
{
        for (int i = 0; i < sizeOfArgv; i++)
        {
                argsWithoutNull[i] = argv[i];
        }
        argsWithoutNull[sizeOfArgv] = NULL;
}

int PrintErr(int enum_error, struct CommandLine structCmd, int status)
{
        if (enum_error == NO_DIRECTORY)
        {
                fprintf(stderr, "Error: No such directory.\n");
        }
        else if (enum_error == TOO_MANY_ARGS)
        {
                fprintf(stderr, "Error: too many process arguments\n");
                return 0;
        }
        else if (enum_error == EXIT_ERROR)
        {
                fprintf(stderr, "Bye...\n");
        }

        fprintf(stderr, "+ completed '%s' [%d]\n", structCmd.cmd, WEXITSTATUS(status));
        return 0;
}

void FindPipeRedir(struct CommandLine *structCmd)
{
        int pipe = 124;
        int redirect = 62;
        for (int i = 0; i < (int)strlen(structCmd->cmd); i++)
        {
                if (structCmd->cmd[i] == pipe)
                        structCmd->isPipe = true;
                if (structCmd->cmd[i] == redirect)
                        structCmd->isRedirect = true;
        }
}

void RemoveTrailingSpace(char cmd[])
{
        char *nl;
        nl = strchr(cmd, '\n');
        if (nl)
                *nl = '\0';
}

void Redirection(const struct CommandLine structCmd)
{
        int status;
        pid_t pid;
        int fd;

        if (structCmd.array_commands != NULL)
        {
                printf("Not Empty\n");
                fflush(stdout);
        }

        pid = fork();

        if (pid == 0)
        {
                /* Child Process*/
                fd = open(structCmd.array_commands[1].args[0], O_WRONLY | O_TRUNC, 0644);
                if (fd == -1)
                {
                        perror("Error: cannot open output file");
                }
                else
                {
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        execvp(structCmd.array_commands[0].args[0], &structCmd.array_commands[0].args[0]);
                        perror("evecvp error in child");
                }
        }
        else if (pid > 0)
        {
                /* Parent Process*/
                waitpid(pid == P_PID, &status, 0);
                PrintErr(0, structCmd, status);
        }
        else
        {
                perror("fork failed");
                exit(1);
        }
}

int Builtin_sls(struct CommandLine structCmd)
{
        struct dirent *dir_entry;
        DIR *directory = opendir(current);
        if (directory == NULL)
        {
                PrintErr(NO_DIRECTORY, structCmd, FAILURE);
                return FAILURE;
        }
        while ((dir_entry = readdir(directory)) != NULL)
        {
                if (dir_entry->d_name[0] != current[0])
                {
                        char cwd[MAX_PATH];
                        getcwd(cwd, sizeof(cwd));
                        struct stat sb;
                        strncat(cwd, "/", sizeof("/"));
                        strncat(cwd, dir_entry->d_name, sizeof(dir_entry->d_name));
                        stat(cwd, &sb);
                        printf("%s (%lld bytes)\n", dir_entry->d_name, (long long)sb.st_size);
                }
        }
        free(dir_entry);
        free(directory);

        return SUCCESS;
}

void Pipe(struct Command process1, struct Command process2)
{
        int fd[2];
        pipe(fd);
        printf("Begin: %s", __func__);
        if (fork() != 0)
        {
                /* Parent */
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                printf("%s\n", process1.args[0]);
                execvp(process1.args[0], &process1.args[0]);
        }
        else
        {
                /* Child */
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                printf("%s\n", process2.args[1]);
                execvp(process2.args[0], &process2.args[0]);
        }
        printf("End: %s", __func__);
}

void Pipeline(struct CommandLine structCmd)
{
        int status;
        int pid_1, pid_2;
        printf("Pipeline, number of commands = %d\n", structCmd.numberOfCommands);
        if (structCmd.numberOfCommands == 2)
        {
                int pid = fork();
                printf("pid = %d\n", pid);
                if (pid != 0)
                {
                        waitpid(pid == P_PID, &status, 0);
                        PrintErr(0, structCmd, status);
                }
                else
                {
                        printf("Child\n");
                        int fd[2];
                        pipe(fd);
                        if (fork() != 0)
                        {
                                /* Parent */
                                close(fd[0]);
                                dup2(fd[1], STDOUT_FILENO);
                                close(fd[1]);
                                //printf("%s\n", structCmd.array_commands[0]);
                                execvp(structCmd.array_commands[0].args[0], &structCmd.array_commands[0].args[0]);
                        }
                        else
                        {
                                /* Child */
                                close(fd[1]);
                                dup2(fd[0], STDIN_FILENO);
                                close(fd[0]);
                                //printf("%s\n", structCmd.array_commands[1].args[0]);
                                execvp(structCmd.array_commands[1].args[0], &structCmd.array_commands[1].args[0]);
                        }
                }
        }

        else if (structCmd.numberOfCommands == 3)
        {
                printf("For 3\n");
                int pid = fork();
                printf("pid = %d\n", pid);
                if (pid > 0)
                {
                        waitpid(pid == P_PID, &status, 0);
                        PrintErr(0, structCmd, status);
                }
                else if (pid == 0)
                {
                        printf("Child_0\n");
                        int fd[2];
                        pipe(fd);
                        pid_1 = fork();
                        if (pid_1 > 0)
                        {
                                /* Parent */
                                close(fd[0]);
                                dup2(fd[1], STDOUT_FILENO);
                                close(fd[1]);
                                //printf("%s\n", structCmd.array_commands[0]);
                                execvp(structCmd.array_commands[0].args[0], &structCmd.array_commands[0].args[0]);
                        }
                        else if (pid_1 == 0)
                        {
                                /* Child */
                                printf("Child_1\n");
                                int fd2[2];
                                pipe(fd2);
                                pid_2 = fork();
                                if (pid_2 > 0)
                                {
                                        close(fd[1]);
                                        dup2(fd[0], STDIN_FILENO);
                                        dup2(fd2[1], STDOUT_FILENO);
                                        close(fd[0]);
                                        close(fd2[1]);
                                        execvp(structCmd.array_commands[1].args[0], &structCmd.array_commands[1].args[0]);
                                }
                                else if (pid_2 == 0)
                                {
                                        printf("Child_2\n");
                                        close(fd[0]);
                                        close(fd[1]);
                                        close(fd2[1]);
                                        dup2(fd2[0], STDIN_FILENO);
                                        close(fd2[0]);
                                        execvp(structCmd.array_commands[2].args[0], &structCmd.array_commands[2].args[0]);
                                }
                                else
                                {
                                        printf("Fork pid_2 error\n");
                                }
                        }
                        else
                        {
                                printf("fork pid_1 error\n");
                        }
                }
                else
                {
                        printf("fork pid error\n");
                }
        }
        printf("End : %s\n", __func__);
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char cmd_original[CMDLINE_MAX];
        char cmd_pl_Copy[CMDLINE_MAX];
        char cmd_rd_Copy[CMDLINE_MAX];
        char cmd_struct[CMDLINE_MAX];
        char cwd[MAX_PATH];
        char *argv[MAX_ARGS];
        char *argPL[MAX_ARGS];
        char *argRD[MAX_ARGS];
        bool exit_bool = false;

        while ((1) && exit_bool == false)
        {
                int status;
                pid_t pid;
                struct CommandLine structCmd;

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
                /*Make 3 versions of cmd for parsing*/
                memcpy(cmd_original, cmd, sizeof(cmd));
                memcpy(cmd_pl_Copy, cmd, sizeof(cmd));
                memcpy(cmd_rd_Copy, cmd, sizeof(cmd));
                memcpy(cmd_struct, cmd, sizeof(cmd));

                /*Remove trailing newline from command line*/
                RemoveTrailingSpace(cmd);
                RemoveTrailingSpace(cmd_pl_Copy);
                RemoveTrailingSpace(cmd_rd_Copy);

                /*Set all the value of arguments to NULL*/
                memset(argv, '\0', sizeof(argv));
                memset(argPL, '\0', sizeof(argPL));
                memset(argRD, '\0', sizeof(argRD));

                /*Get the characters into an array words*/
                cmd_struct[strlen(cmd_struct) - 1] = '\0';
                strcpy(structCmd.cmd, cmd_struct);
                structCmd.isPipe = false;
                structCmd.isRedirect = false;
                FindPipeRedir(&structCmd);
                if(structCmd.cmd[0] != '\0'){

                        if (structCmd.isPipe == false && structCmd.isRedirect == false)
                        {
                                structCmd.numberOfCommands = 1;
                                structCmd.array_commands[0].numberOfArguments = ConvertToWords(cmd, argv, delim_space);
                                if (structCmd.array_commands[0].numberOfArguments == TOO_MANY_ARGS)
                                {
                                        PrintErr(TOO_MANY_ARGS, structCmd, 1);
                                        break;
                                }
                                CopyCharArray(structCmd.array_commands[0].args, argv, structCmd.array_commands[0].numberOfArguments);

                                /* Builtin command */

                                /*exit*/
                                if (!strcmp(structCmd.array_commands[0].args[0], EXIT))
                                {
                                        PrintErr(EXIT_ERROR, structCmd, 0);
                                        exit_bool = true;
                                        break;
                                }

                                /*cd*/
                                if (!strcmp(structCmd.array_commands[0].args[0], CD))
                                {
                                        getcwd(cwd, sizeof(cwd));
                                        if (chdir(structCmd.array_commands[0].args[1]) == -1)
                                        {
                                                PrintErr(NO_DIRECTORY, structCmd, FAILURE);
                                        }
                                        continue;
                                }

                                /*pwd*/
                                if (!strcmp(structCmd.array_commands[0].args[0], PWD))
                                {
                                        getcwd(cwd, sizeof(cwd));
                                        printf("%s\n", cwd);
                                        PrintErr(NO_ERROR, structCmd, SUCCESS);
                                        continue;
                                }

                                /*sls*/
                                if (!strcmp(structCmd.array_commands[0].args[0], SLS))
                                {
                                        int check = Builtin_sls(structCmd);
                                        if (check == SUCCESS)
                                        {
                                                PrintErr(NO_ERROR, structCmd, SUCCESS);
                                        }
                                        continue;
                                }

                                pid = fork();
                                if (pid == 0)
                                {
                                        /* Child Process*/
                                        execvp(structCmd.array_commands[0].args[0], &structCmd.array_commands[0].args[0]);
                                }
                                else if (pid > 0)
                                {
                                        /* Parent Process*/
                                        waitpid(pid == P_PID, &status, 0);
                                        if (WIFEXITED(status) != 0)
                                        {
                                                PrintErr(0, structCmd, status);
                                        }
                                        else
                                        {
                                                perror("evecvp error in child");
                                        }
                                }
                                else
                                {
                                        perror("fork");
                                        exit(1);
                                }
                        }
                        else if (structCmd.isPipe == false && structCmd.isRedirect == true)
                        {

                                structCmd.numberOfCommands = ConvertToWords(cmd_rd_Copy, argRD, delim_redirect);

                                //numberOfCommands Should be = 2
                                char *redirArgsTrim[structCmd.numberOfCommands];
                                char *argvCommandsRedirect[MAX_ARGS];

                                CopyCharArray(redirArgsTrim, argRD, structCmd.numberOfCommands);

                                for (int i = 0; i < structCmd.numberOfCommands; i++)
                                {
                                        structCmd.array_commands[i].numberOfArguments = ConvertToWords(redirArgsTrim[i], argvCommandsRedirect, delim_space);
                                        printf("number of arguments: %d\n", structCmd.array_commands[i].numberOfArguments);
                                        CopyCharArray(structCmd.array_commands[i].args, argvCommandsRedirect, structCmd.array_commands[i].numberOfArguments);
                                        for (int j = 0; j < structCmd.array_commands[i].numberOfArguments; j++)
                                        {
                                                printf("{ %s } ", structCmd.array_commands[i].args[j]);
                                        }
                                }

                                printf("%d", structCmd.numberOfCommands);

                                Redirection(structCmd);

                                printf("\n");
                        }
                        else if (structCmd.isPipe == true && structCmd.isRedirect == false)
                        {

                                //Implementation of piping
                                structCmd.numberOfCommands = ConvertToWords(cmd_pl_Copy, argPL, delim_pipe);

                                char *pipeArgsTrim[structCmd.numberOfCommands];
                                char *argvCommandsPipe[MAX_ARGS];

                                CopyCharArray(pipeArgsTrim, argPL, structCmd.numberOfCommands);

                                for (int i = 0; i < structCmd.numberOfCommands; i++)
                                {
                                        structCmd.array_commands[i].numberOfArguments = ConvertToWords(pipeArgsTrim[i], argvCommandsPipe, delim_space);
                                        CopyCharArray(structCmd.array_commands[i].args, argvCommandsPipe, structCmd.array_commands[i].numberOfArguments);
                                        for (int j = 0; j < structCmd.array_commands[i].numberOfArguments; j++)
                                        {
                                                printf("{ %s } ", structCmd.array_commands[i].args[j]);
                                        }
                                }

                                Pipeline(structCmd);
                                printf("\n");
                        }
                        else
                {
                        //If both:
                        //After checking only 1 redir and less than 3 pipes
                        int sizeOfPipe = ConvertToWords(cmd_pl_Copy, argPL, delim_pipe);
                        int sizeOfRedir = ConvertToWords(cmd_rd_Copy, argRD, delim_redirect);
                        char *pipeArgsTrim[sizeOfPipe];
                        char *redirArgsTrim[sizeOfRedir]; //Should be size = 2

                        CopyCharArray(pipeArgsTrim, argPL, sizeOfPipe);
                        CopyCharArray(redirArgsTrim, argRD, sizeOfRedir);

                        for (int i = 0; i < sizeOfRedir; i++)
                        {
                                printf("[ %s ] ", redirArgsTrim[i]);
                        }
                        printf("%ld", sizeof(redirArgsTrim) / sizeof(char));
                        printf("\n");

                        for (int i = 0; i < sizeOfPipe; i++)
                        {
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
        }

        return EXIT_SUCCESS;
}