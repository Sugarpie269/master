sshell:	sshell.c
	gcc -g -Wall -Wextra -Werror -o sshell sshell.c

.PHONY : clean
clean :
	-rm sshell_exec 