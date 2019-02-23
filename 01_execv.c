#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/wait.h>


int main(){
	pid_t pid;
	//char *const parmList[] = {"/bin/ls", "-l", "/users/gianfranco/Documents/maestria",
      //                         ">", "/users/gianfranco/Desktop/screen", NULL};

	char *const parmList[] = {"ls", "-l", NULL};
	int status;
	pid = fork();

	switch(pid){
		case 0:
			sleep(3);
			execvp("ls", parmList);
			printf("Return not expected. Must be an execv error");
			printf("soy hijo, mi pid es %d, mi padre es %d\n", getpid(), getppid());
//			return 45;
			break;

		case -1:
			perror("error en el fork:");
			break;
		
		default:
			printf("hello from padre %d, hijo es %d\n", getpid(), pid);
			
			pid = wait(&status);			
			assert(pid != -1);
			printf("mi hijo %d, retorno %d (0x%04x)%s\n", pid,
					WEXITSTATUS(status), status,
					WIFSIGNALED(status) ? "signaled" : "muerte natural");

			break;
	}
	
	printf("fin, soy %d\n", getpid());
	return 0;
}
