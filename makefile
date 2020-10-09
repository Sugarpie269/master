sshell_exec:	sshell.c
	gcc -g -Wall -Wextra -Werror -o sshell_exec sshell.c

.PHONY : clean
clean :
	-rm sshell_exec 