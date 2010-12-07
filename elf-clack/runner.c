#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	int pid, status;
	
	if (argc != 2)
		return 1;
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork error: %s\n", strerror(errno));
		return 1;
	} else if (pid == 0)  {
		if (execv(argv[1], argv) < 0)
			fprintf(stderr, "execv error: %s\n", strerror(errno));
	} else {
		while (wait(&status) < 0)
			;
		printf("%d\n", WEXITSTATUS(status));
		printf("%d\n", WIFEXITED(status));
		printf("%d\n", WTERMSIG(status));
	}
	return 0;
}

