#include "mylib.h"

void buildSubmitRequest(submitRequest_t *preq, int argc, char **argv){
	strcpy(preq->its.user, argv[1]);
	strcpy(preq->its.exe, argv[2]);
	preq->n_args = argc - 2;
}

void processSubmitResponse(serverResponse_t *pres){
	printf("Yob with id %d %s \n", pres->subRes.id, pres->subRes.msg);	
}

int main(int argc, char *argv[])
{
    int sockfd;
	int i;
	
	//sockfd = setupClientSocket_UNIX("socketfile");
	sockfd = setupClientSocket_INET();

	commandRequest_t cmd;
	submitRequest_t req;
	serverResponse_t res;

	/* envio el CMD_SUBMIT */
	cmd = CMD_SUBMIT;
	write(sockfd, &cmd, sizeof(cmd));

	/* armo el submitRequest y lo envio */
	buildSubmitRequest(&req, argc, argv);
	write(sockfd, &req, sizeof(req));

	/* envio los argumentos */
	size_t argSz;
	for(i=2; i<argc; i++){
		argSz = strlen(argv[i]);
		write(sockfd, &argSz, sizeof(argSz));
		write(sockfd, argv[i], argSz);
	}

	/* Leo mensaje del server */
	read(sockfd, &res, sizeof(res));
	processSubmitResponse(&res);

    close(sockfd);
    return 0;
}

