# Basic UNIX-like Shell Implementation Utilizing System Calls in C
*Authors: Johnny Huey <jhuey@ucdavis.edu>, Navya Gupta <navgupta@ucdavis.edu>* 

## Introduction
    The provided program has been devised to emulate the experience of a UNIX shell as envisioned by the specifications. It will accept user inputs of any kind and act accordingly to command executions, output redirections, pipeline chaining, and erroneous inputs. Following this, the report will be split into 5 sections to discuss in detail the implementations and processes occurring within the program.

## (1) User Input Parsing: 
    ### Introduction: 
        Representation of the parsed arguments in the command line plays an important role in this program. The representation of the command line is via a struct CommandLine which includes struct Commands[], char cmd[], bool variables, numberOfCommands and indexes of  pipe, redirect and append.  
    ### How: 
        The function void FindPipeRedir(struct CommandLine *structCmd) initializes the bool variables in the struct CommandLine by iterating the structCmd.cmd[] and comparing each character with “|”, “>”, or “ ”. 
        After that, the if..else uses the struct CommandLine bool variables to divide the main into compartments for the different commands. The struct CommandLine is now parsed to the function ConvertToWords(cmd, argv, delim). The function splits the char cmd[] using the delim provided and ignores tokens which only have space using the function bool CheckIfAllSpace(char *argv). The bool CheckIfAllSpace(char *argv) iterates the cmd and compares the argv[] to ASCII value of space. It returns false if argv[i] is not space. But, if the command has more than one process than the function ConvertToWords(cmd, argv, delim) splits the char cmd[] using the delim provided again. Hence after the second call to the function ConvertToWords(cmd, argv, delim), the struct CommandLine can now store the arguments into Command[i].args[j].  

## (2) Custom Builtins and Execution:  
    ### Introduction: The execution of the builtin commands exit, cd, pwd and sls.
    ### How:
        The if...else would check for the structCmd bool variables : !structCmd.isPipe && !structCmd.isRedirect && !structCmd.isRedirAppend. It would be true for this case because the structCmd only contains one process. 
        EXIT: It is executed in the main with the help of the variable exit_bool which turns true if the string comparison of the structCmd.array_commands[0].args[0] and macro EXIT returns 0. The value of the exit_bool is used in while ((1) && exit_bool == false). Therefore, it exits when the exit_bool is true. 
        The rest of the builtin commands are executed in the function void BuiltinCommands(struct CommandLine *structCmd). In the function, the code differentiates between the builtin Commands by using strcmp to compare them with their respective defined macros CD, PWD and SLS. 
        CD: Syscall function chdir() will take structCmd->array_commands[0].args[1] which contains the path to change the working directory for the user. 
        PWD:Syscall function getcwd() obtains the current working directory and places the content into its input variable cwd, which gets displayed onto the terminal.
        SLS: The current directory is opened and read in a while loop using directory = opendir(current) and  dir_entry = readdir(directory) respectively until dir_entry is NULL. lstat() uses the dir_entry and struct stat to extract the status of the file or directory. 

        If these builtin commands are executed then structCmd->is_builtin == true. The main checks structCmd->is_builtin value. If true, program executes continue. Else, the fork will create a parent and child process. The child process executes execvp(structCmd.array_commands[0].args[0], &structCmd.array_commands[0].args[0]) while the parent process waits to collect the child and print the status of the child using waitpid(). 

## (3) Output Redirection and Appending:
	### Introduction:
	    Output Redirection and Redirection Appending both require interaction with some file indicated by the user input. In addition to executing a command with execvp(), the results require some way to shift itself into the specified file. Thus, open() and execvp() play a vital role in the function Redirection(). 

	### How:
        Function void Redirection(const struct CommandLine structCmd, int rd_mode) runs when struct CommandLine StructCmd  has .isRedirect or .isRedirAppend was set to true, (via FindPipeRedir()). For rd_mode, enum macro TRUNCT is passed in when .isRedirect is true and enum macro APPD for when .isRedirAppend is true. 
        Within Redirect(), the program creates a child via fork(). The child takes the filename indicated by structCmd.array_commands[1].args[0] and opens it for writing with open(structCmd.array_commands[1].args[0], O_WRONLY | O_CREAT, 0644) to create a file descriptor fd. Here, rd_mode will determine if the file’s open() should include Truncation mode macro O_TRUNC or Appending mode macro O_APPEND. Then, the child proceeds to link STDOUT to the file described by fd with dup2(fd, STDOUT_FILENO). Lastly, the child runs execvp() on structCmd.array_commands[0].args[0]. The parent waits for the child process to finish via waitpid(), prints out the child’s exit status WIFEXITSTATUS(status), and returns back to main() to restart the user input process. 


## (4) Pipelining:
    ### Introduction:
        Pipelined commands handle multiple instances of executions via execcp(). Each process also requires a method to communicate data to the next execution, thus the use of fork(), pipe(), and execvp() are carefully planned in the function Pipeline(). 

    ### How:
        Function Pipeline(struct CommandLine structCmd, int numberOfPipeCommands) takes in struct CommandLine structCmd and runs when structCmd.isPipeline, structCmd.isPipeBeforeRedirect, or structCmd.isPipeBeforeAppend was set to true via FindPipeRedir(). Pipeline() initializes a 2D int array variable pipeArray of size [numberOfPipeCommands -1][2], where each array element of size 2 is turned into pipe fd ports via pipe(). Then, a for loop executes i times, where i = numberOfPipeCommands, (number of commands = number of executions). Each iteration will fork() a child process to do the following: read in STDIN from the previous pipe pipeArray[i - 1][READ], (when it is not the first iteration), and write STDOUT to the next pipe pipeArray[i][WRITE], (when it isn’t the last iteration). Corresponding STDIN/OUT file descriptors are connected via dup2() and all other pipe ports in pipeArray are closed via close() to prevent unintentional pipe blocking. Lastly, each child will execute structCmd.array_commands[i] via execvp() and corresponding results will stream through the pipeline.
        One parent process exists outside of the loop to claim “zombified” children. “while ((corpse = waitpid(0, &status, 0)) > 0)”, makes the parent wait until all children with the same Group PID are claimed, storing the children’s PID in the variable int corpse, (Source: Leffler, Response to Question). Statuses are stored as int member values of struct Hash and corpse values are stored as Hash int member key. These structs are stored in a global static array of struct Hash called statusArray that can contain up to MAX_ARGS hash elements. This array eventually undergoes qsort() by lowest key value because we want to print the children processes in order. A completion message will print these statuses, and the program returns back to the user input phase.

## Special Case: Pipeline and Output Redirection:
	### How:
        Pipelines are allowed to have 1 output redirection at the end of a pipeline statement. When structCmd.isPipeBeforeRedirect or structCmd.isPipeBeforeAppend is true, function PipeAndRedirection() runs. Assuming that all relevant errors have been handled, the input can safely use Pipeline() to execute each structCmd.array_commands[i] up to the last, structCmd.array_commands[structCmd.numberOfCommands - 1], which will be a filename for output redirection (or redirection appending). The corresponding file will be opened in the appropriate mode (O_TRUCT or O_APPEND) and after using dup2() and closing the fd created by open(), the results of Pipeline() get written.  

## Ending Remarks:
	This shell program relies on several assumptions listed in the specifications, and thus, may not be able to fully emulate a real UNIX shell. There is a multiple output redirections limit set to 1 which a UNIX shell can handle but this program cannot. Then there is the hardcoded hash table sorting for child process status collection, which relies on the 3 pipeline assumption given by the specification. (WIP)


Cited Sources:
1. Leffler, J. (2013), Answer to “What is the proper way to pipe when making a shell in C”, https://stackoverflow.com/questions/15673333/what-is-the-proper-way-to-pipe-when-making-a-shell-in-c
