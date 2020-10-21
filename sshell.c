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

#define EXIT "exit"
#define CD "cd"
#define PWD "pwd"
#define SLS "sls"
#define delim_space " "
#define delim_pipe "|"
#define delim_redirect ">"
#define delim_pipe_and_redirect "|>"
#define current "."

static int statusArray[MAX_ARGS];

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
        COMMAND_NOT_FOUND_PIPE = -12,
};

enum LAUNCHING_ERRORS
{
        ERROR_EXECVP = 255,
        CANNOT_CD_INTO_DIRECTORY = -9,
};

enum DELIMS
{
        SPACE = 32,
        PIPE = 124,
        REDIRECT = 62,
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
        bool isPipeBeforeCmd;
        bool isPipeBeforeRedirect;
        bool isPipeBeforeAppend;
        bool isRedirectBeforePipe;
        bool isAppendBeforePipe;
        bool isRedirBeforeCmd;
        bool is_builtin;
        bool to_many_args;

        int numberOfCommands;
        int pipe_index;
        int redirect_index;
        int redirect_append_index;
};

bool CheckIfAllSpace(char *argv)
{
        for (int i = 0; i < (int)strlen(argv); i++)
        {
                if (argv[i] != SPACE)
                {
                        return false;
                }
        }
        return true;
}
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

                if (CheckIfAllSpace(token) == false)
                {
                        argv[i] = token;
                        //printf("ARGV[%d] = %s | ", i, argv[i]);
                        i++;
                }

                token = strtok(NULL, delim);
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
        else if (enum_error == COMMAND_NOT_FOUND_PIPE)
        {
            fprintf(stderr, "Error: command not found\n");
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
        int lastCharIndex = -1;
        for (int i = 0; i < (int)strlen(structCmd->cmd); i++)
        {
                if (lastCharIndex == -1 && structCmd->cmd[i] == REDIRECT)
                {
                        structCmd->isRedirBeforeCmd = true;
                        break;
                }
                else if (lastCharIndex == -1 && structCmd->cmd[i] == PIPE)
                {
                        structCmd->isPipeBeforeCmd = true;
                        break;
                }
                else if (structCmd->cmd[i] == PIPE)
                {
                        structCmd->isPipe = true;
                        structCmd->pipe_index = i;
                }
                else if (structCmd->cmd[lastCharIndex] == REDIRECT && structCmd->cmd[i] == REDIRECT)
                {
                        structCmd->isRedirAppend = true;
                        structCmd->isRedirect = false;
                        structCmd->redirect_append_index = lastCharIndex;
                }
                else if (structCmd->cmd[i] == REDIRECT)
                {
                        structCmd->isRedirect = true;
                        structCmd->redirect_index = i;
                }

                if (structCmd->cmd[i] != SPACE)
                {
                        lastCharIndex = i;
                }
        }

        if (structCmd->isPipe == true && structCmd->isRedirect == true && (structCmd->pipe_index < structCmd->redirect_index))
        {
                structCmd->isPipeBeforeRedirect = true;
        }else if(structCmd->isPipe == true && structCmd->isRedirAppend == true &&  (structCmd->pipe_index < structCmd->redirect_append_index)){
                structCmd->isPipeBeforeAppend = true;
        }
        else if (structCmd->isPipe == true && structCmd->isRedirect == true && (structCmd->pipe_index > structCmd->redirect_index))
        {
                structCmd->isRedirectBeforePipe = true;
        }else if(structCmd->isPipe == true && structCmd->isRedirAppend == true &&  (structCmd->pipe_index > structCmd->redirect_append_index)){
                structCmd->isAppendBeforePipe = false;
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
                        exit(1);
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
                waitpid(pid == P_PID, &status, SUCCESS);
                PrintErr(SUCCESS, structCmd, status);
        }
        else
        {
                perror("fork failed");
                exit(1);
        }
}

void PrintArrayStatus(struct CommandLine structCmd, int sizeOfStatusArray)
{
        fprintf(stderr, "+ completed '%s' ", structCmd.cmd);

        for (int i = 0; i < sizeOfStatusArray; i++)
        {
                printf("[%d]", WEXITSTATUS(statusArray[i]));
        }
        printf("\n");
}

void Pipeline(struct CommandLine structCmd, int numberOfPipeCommands)
{
        int status;
        int corpse;
        int k = 0;
        pid_t pid;


        int pipeArray[numberOfPipeCommands - 1][2];
        for (int i = 0; i < numberOfPipeCommands - 1; i++)
        {
                pipe(pipeArray[i]);
        }


        for (int i = 0; i < numberOfPipeCommands; i++)
        {

                pid = fork();


                if (pid == 0)
                {

                        if (i == 0)
                        {

                                //On the first cmd chunk, don't need stdin
                                dup2(pipeArray[i][WRITE], STDOUT_FILENO);
                        }
                        else if (i > 0 && i != numberOfPipeCommands - 1)
                        {

                                //On the inside of pipeline, need the things writtine in STDOUT to be read into STDIN
                                dup2(pipeArray[i - 1][READ], STDIN_FILENO);
                                dup2(pipeArray[i][WRITE], STDOUT_FILENO);
                        }
                        else if (i == numberOfPipeCommands - 1)
                        {

                                //On the last chunk, don't need to pipe stdout
                                dup2(pipeArray[i - 1][READ], STDIN_FILENO);
                        }
                        else
                        {
                                fprintf(stderr, "EM: [i] is negative. ");
                        }

                        for (int j = 0; j < numberOfPipeCommands - 1; j++)
                        { 
                                //closing pipes in child
                                close(pipeArray[j][READ]);
                                close(pipeArray[j][WRITE]);
                        }

                        //fprintf(stderr, "EM: Done with dup2 and closing, now exec.\n");
                        int res = execvp(structCmd.array_commands[i].args[0], &structCmd.array_commands[i].args[0]);

                        if (res == -1)
                        {
                            PrintErr(COMMAND_NOT_FOUND_PIPE, structCmd, 0);
                            res = 1;
                        }
                        exit(res);

                }

                else if (pid < 0)
                {
                        printf("Fork Error. Exiting...\n");
                        exit(-1);
                }
        }

        // closing previous pipes in parent
        for (int j = 0; j < numberOfPipeCommands - 1; j++)
        { 
                close(pipeArray[j][0]);
                close(pipeArray[j][1]);
        }

        while ((corpse = waitpid(0, &status, 0)) > 0)
        {
                statusArray[k] = (int)(status);
                k++;
        }
}

void PipeAndRedirection(struct CommandLine structCmd)
{
        int fd;
        int status;
        pid_t pid;
        if (structCmd.array_commands != NULL)
        {

                fflush(stdout);
        }

        pid = fork();

        if (pid == 0)
        {
                if(structCmd.isPipeBeforeRedirect){
                        fd = open(structCmd.array_commands[structCmd.numberOfCommands - 1].args[0], O_WRONLY | O_TRUNC | O_CREAT, 0644);
                }else if(structCmd.isPipeBeforeAppend){
                        fd = open(structCmd.array_commands[structCmd.numberOfCommands - 1].args[0], O_WRONLY | O_APPEND| O_CREAT, 0644);
                }
                if (fd == -1)
                {
                        PrintErr(CANNOT_OPEN_OUTPUT_FILE, structCmd, FAILURE);
                        exit(1);
                }
                else
                {

                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        Pipeline(structCmd, structCmd.numberOfCommands - 1);
                        exit(1);
                }
        }
        else if (pid > 0)
        {
                waitpid(pid == P_PID, &status, 0);
                PrintArrayStatus(structCmd, structCmd.numberOfCommands);
                //PrintErr(NO_ERROR, structCmd, SUCCESS);
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
                if (dir_entry->d_name[0] != current[0] && dir_entry != NULL)
                {
                        struct stat sb;
                        lstat(dir_entry->d_name, &sb);
                        printf("%s (%lld bytes)\n", dir_entry->d_name, (long long)sb.st_size);
                }
        }

        free(dir_entry);
        free(directory);

        return SUCCESS;
}

void BuiltinCommands(struct CommandLine *structCmd)
{
        /*cd*/
        if (!strcmp(structCmd->array_commands[0].args[0], CD))
        {
                char cwd[CMDLINE_MAX];
                getcwd(cwd, sizeof(cwd));
                if (chdir(structCmd->array_commands[0].args[1]) == -1)
                {
                        PrintErr(CANNOT_CD_INTO_DIRECTORY, *structCmd, FAILURE);
                }
                else
                {
                        PrintErr(NO_ERROR, *structCmd, SUCCESS);
                }
                structCmd->is_builtin = true;
        }

        /*pwd*/
        if (!strcmp(structCmd->array_commands[0].args[0], PWD))
        {
                char cwd[CMDLINE_MAX];
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

void Init_struct_cmd(struct CommandLine *structCmd)
{
        structCmd->is_builtin = false;
        structCmd->isPipe = false;
        structCmd->isPipeBeforeCmd = false;
        structCmd->isRedirBeforeCmd = false;
        structCmd->to_many_args = false;
        structCmd->isRedirAppend = false;
        structCmd->isRedirect = false;
        structCmd->isPipeBeforeRedirect = false;
        structCmd->isRedirectBeforePipe = false;
        structCmd->isPipeBeforeAppend = false;
        structCmd->isAppendBeforePipe = false;

        structCmd->pipe_index = 0;
        structCmd->redirect_index = 0;
        structCmd->redirect_append_index = 0;
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
        int res = 0;
        while ((1) && exit_bool == false)
        {
                int status;
                pid_t pid;
                struct CommandLine structCmd;

                /* Print prompt */
                printf("sshell@ucd$ ");
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

                if (structCmd.isPipeBeforeCmd || structCmd.isRedirBeforeCmd)
                {
                        PrintErr(MISSING_COMMAND, structCmd, FAILURE);
                        continue;
                }
                else if (structCmd.isRedirectBeforePipe || structCmd.isAppendBeforePipe)
                {
                        PrintErr(MISCLOCATED_OUTPUT_REDIRECTION, structCmd, FAILURE);
                        continue;
                }

                /*if cmd is not empty*/
                if (structCmd.cmd[0] != '\0')
                {

                        if (!structCmd.isPipe && !structCmd.isRedirect && !structCmd.isRedirAppend)
                        {
                                structCmd.numberOfCommands = 1;
                                structCmd.array_commands[0].numberOfArguments = ConvertToWords(cmd, argv, delim_space);

                                /*check for number of arguments*/
                                if (structCmd.array_commands[0].numberOfArguments >= MAX_ARGS)
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
                                if (structCmd.is_builtin == true)
                                {
                                        continue;
                                }

                                pid = fork();
                                if (pid == 0)
                                {
                                        /* Child Process*/
                                        res = execvp(structCmd.array_commands[0].args[0], &structCmd.array_commands[0].args[0]);
                                        if (res == -1)
                                        {
                                                PrintErr(COMMAND_NOT_FOUND, structCmd, status);
                                        }
                                        exit(res);
                                }
                                else if (pid > 0)
                                {
                                        /* Parent Process*/
                                        waitpid(pid == P_PID, &status, 0);
                                        if (WEXITSTATUS(status) != 0 && WEXITSTATUS(status) != ERROR_EXECVP)
                                        {
                                                /*Child exited normally, but returns an exit status defined by execvp results*/
                                                PrintErr(NO_ERROR, structCmd, status);
                                        }
                                        else if (WEXITSTATUS(status) == 0 && WEXITSTATUS(status) != ERROR_EXECVP)
                                        {
                                                /*Child normal exit and success*/
                                                PrintErr(NO_ERROR, structCmd, status);
                                        }
                                }
                                else
                                {
                                        perror("TEMP: fork error");
                                        exit(1);
                                }
                        }
                        else if (!structCmd.isPipe && (structCmd.isRedirect || structCmd.isRedirAppend))
                        {
                                /*Redirection and Appending*/

                                structCmd.numberOfCommands = ConvertToWords(cmd_rd_Copy, argRD, delim_redirect);

                                if (structCmd.numberOfCommands >= MAX_ARGS)
                                {
                                        structCmd.to_many_args = true;
                                        PrintErr(TOO_MANY_ARGS, structCmd, FAILURE);
                                        continue;
                                }
                                else if (structCmd.numberOfCommands != 2)
                                {
                                        PrintErr(NO_OUTPUT_FILE, structCmd, FAILURE);
                                        continue;
                                }

                                char *redirArgsTrim[structCmd.numberOfCommands];
                                char *argvCommandsRedirect[MAX_ARGS];

                                CopyCharArray(redirArgsTrim, argRD, structCmd.numberOfCommands);

                                for (int i = 0; i < structCmd.numberOfCommands; i++)
                                {
                                        structCmd.array_commands[i].numberOfArguments = ConvertToWords(redirArgsTrim[i], argvCommandsRedirect, delim_space);
                                        if (structCmd.array_commands[i].numberOfArguments >= MAX_ARGS)
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
                        else if (structCmd.isPipe && !structCmd.isRedirect && !structCmd.isRedirAppend)
                        {

                                /*Piping*/

                                structCmd.numberOfCommands = ConvertToWords(cmd_pl_Copy, argPL, delim_pipe);

                                char *pipeArgsTrim[structCmd.numberOfCommands];
                                char *argvCommandsPipe[MAX_ARGS];

                                CopyCharArray(pipeArgsTrim, argPL, structCmd.numberOfCommands);

                                for (int i = 0; i < structCmd.numberOfCommands; i++)
                                {
                                        structCmd.array_commands[i].numberOfArguments = ConvertToWords(pipeArgsTrim[i], argvCommandsPipe, delim_space);
                                        if (structCmd.array_commands[i].numberOfArguments >= MAX_ARGS)
                                        {
                                                structCmd.to_many_args = true;
                                                break;
                                        }
                                        CopyCharArray(structCmd.array_commands[i].args, argvCommandsPipe, structCmd.array_commands[i].numberOfArguments);
                                }

                                if (structCmd.to_many_args == false && structCmd.isPipe == true)
                                {
                                        Pipeline(structCmd, structCmd.numberOfCommands);
                                        PrintArrayStatus(structCmd, structCmd.numberOfCommands);
                                }
                                else
                                {
                                        PrintErr(TOO_MANY_ARGS, structCmd, FAILURE);
                                        continue;
                                }
                        }
                        else if (structCmd.isPipeBeforeRedirect || structCmd.isPipeBeforeAppend)
                        {

                                structCmd.numberOfCommands = ConvertToWords(cmd_pl_Copy, argPL, delim_pipe_and_redirect);

                                char *pipeAndRedirectArgsTrim[structCmd.numberOfCommands];
                                char *argvCommandsPipe[MAX_ARGS];

                                CopyCharArray(pipeAndRedirectArgsTrim, argPL, structCmd.numberOfCommands);

                                for (int i = 0; i < structCmd.numberOfCommands; i++)
                                {
                                        structCmd.array_commands[i].numberOfArguments = ConvertToWords(pipeAndRedirectArgsTrim[i], argvCommandsPipe, delim_space);
                                        if (structCmd.array_commands[i].numberOfArguments >= MAX_ARGS)
                                        {
                                                structCmd.to_many_args = true;
                                                break;
                                        }
                                        CopyCharArray(structCmd.array_commands[i].args, argvCommandsPipe, structCmd.array_commands[i].numberOfArguments);
                                }
                                PipeAndRedirection(structCmd);
                        }
                }
        }

        return EXIT_SUCCESS;
}