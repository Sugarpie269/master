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
#include <errno.h>

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
        COMMAND_NOT_FOUND = -10,
        NO_REDIR_NO_PIPE = -11,
};

enum LAUNCHING_ERRORS
{
        UNKNOWN_COMMAND = 2,
        CANNOT_CD_INTO_DIRECTORY = -9,
};

enum DELIMS
{
        SPACE = 0,
        PIPE = 1,
        REDIRECT = 2,
        NUMBER_OF_DELIMS = 3,
};

enum STATUS
{
        SUCCESS = 0,
        FAILURE = 1,
};

enum FD
{
        READ = 0,
        WRITE = 1,
        NUMBER_OF_FD = 2,
};

enum REDIR_MODE
{
        TRUNCT = 0,
        APPD = 1,
};

struct Command
{
        char *args[MAX_ARGS];
        int numberOfArguments;
};

struct CommandLine
{
        struct Command array_commands[MAX_ARGS];
        char cmd[CMDLINE_MAX];
        bool isRedirect;
        bool isPipe;
        bool isRedirAppend;
        int numberOfCommands;
        bool to_many_args;
        bool redirBeforeCmd;
        bool pipeBeforeCmd;
        bool is_builtin;
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
                token = strtok(NULL, delim);
                i++;
        }
        if (i == 1 && delim[0] != delim_space[0])
        {
                return NO_REDIR_NO_PIPE;
        }
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
        if (enum_error == TOO_MANY_ARGS)
        {
                fprintf(stderr, "Error: too many process arguments\n");
                return 0;
        }
        else if (enum_error == MISSING_COMMAND)
        {
                fprintf(stderr, "Error: missing command\n");
                return 0;
        }
        else if (enum_error == NO_OUTPUT_FILE)
        {
                fprintf(stderr, "Error: no output file\n");
                return 0;
        }
        else if (enum_error == CANNOT_OPEN_OUTPUT_FILE)
        {
                fprintf(stderr, "Error: can not open output file\n");
                return 0;
        }
        else if (enum_error == MISCLOCATED_OUTPUT_REDIRECTION)
        {
                fprintf(stderr, "Error: mislocated output redirection\n");
                return 0;
        }
        else if (enum_error == NO_DIRECTORY)
        {
                fprintf(stderr, "Error: No such directory.\n");
        }
        else if (enum_error == EXIT_ERROR)
        {
                fprintf(stderr, "Bye...\n");
        }
        else if (enum_error == CANNOT_CD_INTO_DIRECTORY)
        {
                fprintf(stderr, "Error: cannot cd into directory\n");
        }
        else if (enum_error == COMMAND_NOT_FOUND)
        {
                fprintf(stderr, "Error: command not found\n");
                fprintf(stderr, "+ completed '%s' [%d]\n", structCmd.cmd, 1);
                return 0;
        }
        if (status == FAILURE)
        {
                fprintf(stderr, "+ completed '%s' [%d]\n", structCmd.cmd, 1);
        }
        else
        {
                fprintf(stderr, "+ completed '%s' [%d]\n", structCmd.cmd, WEXITSTATUS(status));
        }
        return 0;
}

void FindPipeRedir(struct CommandLine *structCmd)
{
        int pipe = 124;
        int redirect = 62;

        for (int i = 0; i < (int)strlen(structCmd->cmd) - 1; i++)
        {
                if (structCmd->cmd[0] == redirect)
                {
                        structCmd->numberOfCommands = true;
                        break;
                }
                else if (structCmd->cmd[0] == pipe)
                {
                        structCmd->pipeBeforeCmd = true;
                        break;
                }
                else if (structCmd->cmd[i] == pipe)
                {
                        structCmd->isPipe = true;
                }
                else if (structCmd->cmd[i] == redirect && structCmd->cmd[i + 1] == redirect)
                {
                        structCmd->isRedirAppend = true;
                }
                else if (structCmd->cmd[i] == redirect)
                {
                        structCmd->isRedirect = true;
                        i++;
                }
        }
}

void RemoveTrailingSpace(char cmd[])
{
        char *nl;
        nl = strchr(cmd, '\n');
        if (nl)
                *nl = '\0';
}

void Redirection(const struct CommandLine structCmd, int rd_mode)
{
        int status;
        pid_t pid;
        int fd;

        if (structCmd.array_commands != NULL)
        {
                fflush(stdout);
        }

        pid = fork();

        if (pid == 0)
        {
                /* Child Process*/
                if (rd_mode == TRUNCT)
                {
                        fd = open(structCmd.array_commands[1].args[0], O_WRONLY | O_TRUNC | O_CREAT, 0644);
                }
                else
                {
                        fd = open(structCmd.array_commands[1].args[0], O_WRONLY | O_APPEND | O_CREAT, 0644);
                }

                if (fd == -1)
                {
                        PrintErr(CANNOT_OPEN_OUTPUT_FILE, structCmd, FAILURE);
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

void BuiltinCommands(struct CommandLine *structCmd)
{
        char cwd[MAX_PATH];
        /*cd*/
        if (!strcmp(structCmd->array_commands[0].args[0], CD))
        {
                getcwd(cwd, sizeof(cwd));
                if (chdir(structCmd->array_commands[0].args[1]) == -1)
                {
                        PrintErr(CANNOT_CD_INTO_DIRECTORY, *structCmd, FAILURE);
                }
                structCmd->is_builtin = true;
        }

        /*pwd*/
        if (!strcmp(structCmd->array_commands[0].args[0], PWD))
        {
                getcwd(cwd, sizeof(cwd));
                printf("%s\n", cwd);
                PrintErr(NO_ERROR, *structCmd, SUCCESS);
                structCmd->is_builtin = true;
        }

        /*sls*/
        if (!strcmp(structCmd->array_commands[0].args[0], SLS))
        {
                int check = Builtin_sls(*structCmd);
                if (check == SUCCESS)
                {
                        PrintErr(NO_ERROR, *structCmd, SUCCESS);
                }
                structCmd->is_builtin = true;
        }
}

void Init_struct_cmd(struct CommandLine *structCmd){
        structCmd->is_builtin = false;
        structCmd->isPipe = false;
        structCmd->pipeBeforeCmd = false;
        structCmd->redirBeforeCmd = false;
        structCmd->to_many_args = false;
        structCmd->isRedirAppend = false;
        structCmd->isRedirect = false;
}


int main(void)
{
        char cmd[CMDLINE_MAX];
        char cmd_original[CMDLINE_MAX];
        char cmd_pl_Copy[CMDLINE_MAX];
        char cmd_rd_Copy[CMDLINE_MAX];
        char cmd_struct[CMDLINE_MAX];
        char *argv[MAX_ARGS];
        char *argPL[MAX_ARGS];
        char *argRD[MAX_ARGS];
        bool exit_bool = false;
        extern int errno;

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

                /*Get the cmd without the new line into the structCmd*/
                cmd_struct[strlen(cmd_struct) - 1] = '\0';
                strcpy(structCmd.cmd, cmd_struct);

                /*Initialize the bool variables for structCmd*/
                Init_struct_cmd(&structCmd);

                /*Find what type of cmd do we have*/
                FindPipeRedir(&structCmd);

                if (structCmd.pipeBeforeCmd || structCmd.redirBeforeCmd)
                {
                        PrintErr(MISSING_COMMAND, structCmd, FAILURE);
                        continue;
                }

                /*if cmd is not empty*/
                if (structCmd.cmd[0] != '\0')
                {

                        if (structCmd.isPipe == false && structCmd.isRedirect == false)
                        {
                                structCmd.numberOfCommands = 1;
                                structCmd.array_commands[0].numberOfArguments = ConvertToWords(cmd, argv, delim_space);

                                /*check for number of arguments*/
                                if (structCmd.numberOfCommands == MAX_ARGS)
                                {
                                        PrintErr(TOO_MANY_ARGS, structCmd, FAILURE);
                                        continue;
                                }

                                /*Remove the null characters from the array*/
                                CopyCharArray(structCmd.array_commands[0].args, argv, structCmd.array_commands[0].numberOfArguments);

                                /*exit*/
                                if (!strcmp(structCmd.array_commands[0].args[0], EXIT))
                                {
                                        PrintErr(EXIT_ERROR, structCmd, 0);
                                        exit_bool = true;
                                        break;
                                }

                                /* Builtin command */
                                BuiltinCommands(&structCmd);
                                if(structCmd.is_builtin == true){
                                        continue;
                                }

                                pid = fork();
                                if (pid == 0)
                                {
                                        /* Child Process*/
                                        execvp(structCmd.array_commands[0].args[0], &structCmd.array_commands[0].args[0]);

                                        /*errno == 2 when command not found*/
                                        exit(errno);
                                }
                                else if (pid > 0)
                                {
                                        /* Parent Process*/
                                        waitpid(pid == P_PID, &status, 0);
                                        if (WEXITSTATUS(status) == UNKNOWN_COMMAND)
                                        {
                                                PrintErr(COMMAND_NOT_FOUND, structCmd, COMMAND_NOT_FOUND);
                                        }
                                        else if (WEXITSTATUS(status) != 0)
                                        {
                                                //Child exited normally, but returns an exit status defined by execvp results
                                                PrintErr(NO_ERROR, structCmd, status);
                                        }
                                        else
                                        {
                                                //Child normal exit and success
                                                PrintErr(NO_ERROR, structCmd, status);
                                        }
                                }
                                else
                                {
                                        perror("TEMP: fork error");
                                        exit(1);
                                }
                        }
                        else if (structCmd.isPipe == false && (structCmd.isRedirect == true || structCmd.isRedirAppend == true))
                        {

                                structCmd.numberOfCommands = ConvertToWords(cmd_rd_Copy, argRD, delim_redirect);

                                if (structCmd.numberOfCommands == MAX_ARGS)
                                {
                                        structCmd.to_many_args = true;
                                        PrintErr(TOO_MANY_ARGS, structCmd, FAILURE);
                                        continue;
                                }

                                char *redirArgsTrim[structCmd.numberOfCommands];
                                char *argvCommandsRedirect[MAX_ARGS];

                                CopyCharArray(redirArgsTrim, argRD, structCmd.numberOfCommands);

                                for (int i = 0; i < structCmd.numberOfCommands; i++)
                                {
                                        structCmd.array_commands[i].numberOfArguments = ConvertToWords(redirArgsTrim[i], argvCommandsRedirect, delim_space);
                                        if (structCmd.array_commands[i].numberOfArguments == MAX_ARGS)
                                        {
                                                structCmd.to_many_args = true;
                                                break;
                                        }

                                        CopyCharArray(structCmd.array_commands[i].args, argvCommandsRedirect, structCmd.array_commands[i].numberOfArguments);
                                }

                                if (structCmd.to_many_args == false && structCmd.isRedirAppend == true)
                                {
                                        Redirection(structCmd, APPD);
                                }
                                else if (structCmd.to_many_args == false && structCmd.isRedirect == true)
                                {
                                        Redirection(structCmd, TRUNCT);
                                }
                                else
                                {
                                        PrintErr(TOO_MANY_ARGS, structCmd, FAILURE);
                                        continue;
                                }
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
                                        if (structCmd.array_commands[i].numberOfArguments == MAX_ARGS)
                                        {
                                                structCmd.to_many_args = true;
                                                break;
                                        }
                                        CopyCharArray(structCmd.array_commands[i].args, argvCommandsPipe, structCmd.array_commands[i].numberOfArguments);
                                }

                                if (structCmd.to_many_args == false && structCmd.isPipe == true)
                                {
                                        Pipeline(structCmd);
                                }
                                else
                                {
                                        PrintErr(TOO_MANY_ARGS, structCmd, FAILURE);
                                        continue;
                                }
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