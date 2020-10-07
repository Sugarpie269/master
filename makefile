sshell_exec:	sshell.c
	gcc -g -Wall -o sshell_exec sshell.c
clean:	$(RM) sshell 