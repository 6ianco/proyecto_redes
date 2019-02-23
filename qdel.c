#include "mylib.h"

void processDelResponse(serverResponse_t *pres){
	printf("Job with id [%d] %s\n",
	pres->delRes.id,
	pres->delRes.msg);
}

int main(int argc, char *argv[])
{
    int sockfd;
	
	//sockfd = setupClientSocket_UNIX("socketfile");
	sockfd = setupClientSocket_INET();

	commandRequest_t cmd;
	deleteRequest_t jobId;
	serverResponse_t res;

	cmd = CMD_DELETE;
	jobId = atoi(argv[1]);
	write(sockfd, &cmd, sizeof(cmd));
	write(sockfd, &jobId, sizeof(jobId));
	read(sockfd, &res, sizeof(res));
    close(sockfd);
	processDelResponse(&res);
    return 0;
}

