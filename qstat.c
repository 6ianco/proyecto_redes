#include "mylib.h"

void printHeader(){
	printf("---------------------------------------------------------------------------------------\n");
	printf("job  user        program     submit time            start time             running time\n");
	printf("---------------------------------------------------------------------------------------\n");	
}

void printRQR(serverResponse_t *pres){

	printf("%03d  %-10s  %-10s  [%02d/%02d/%04d %02d:%02d:%02d]  [%02d/%02d/%04d %02d:%02d:%02d]  [%03d:%02d:%02d]\n",
	pres->qryRes.job.id,
	pres->qryRes.job.its.user,
	pres->qryRes.job.its.exe,

	pres->qryRes.job.reg.submit.tm_mday,
	pres->qryRes.job.reg.submit.tm_wday,
	pres->qryRes.job.reg.submit.tm_year+1900,
	pres->qryRes.job.reg.submit.tm_hour,
	pres->qryRes.job.reg.submit.tm_min,
	pres->qryRes.job.reg.submit.tm_sec,

	pres->qryRes.job.reg.start.tm_mday,
	pres->qryRes.job.reg.start.tm_wday,
	pres->qryRes.job.reg.start.tm_year+1900,
	pres->qryRes.job.reg.start.tm_hour,
	pres->qryRes.job.reg.start.tm_min,
	pres->qryRes.job.reg.start.tm_sec,

	pres->qryRes.job.run.hours,
	pres->qryRes.job.run.minutes,
	pres->qryRes.job.run.seconds);	
}

void printQQR(serverResponse_t *pres){

	printf("%03d  %-10s  %-10s  [%02d/%02d/%04d %02d:%02d:%02d]  [----- waiting -----]  [%03d:%02d:%02d]\n",
	pres->qryRes.job.id,
	pres->qryRes.job.its.user,
	pres->qryRes.job.its.exe,

	pres->qryRes.job.reg.submit.tm_mday,
	pres->qryRes.job.reg.submit.tm_wday,
	pres->qryRes.job.reg.submit.tm_year+1900,
	pres->qryRes.job.reg.submit.tm_hour,
	pres->qryRes.job.reg.submit.tm_min,
	pres->qryRes.job.reg.submit.tm_sec,

	pres->qryRes.job.run.hours,
	pres->qryRes.job.run.minutes,
	pres->qryRes.job.run.seconds);	
}

/* QQR: Queue Query Response */
void processQQR(int fd){
	serverResponse_t res;
	boolean_t isEmpty;
	read(fd, &isEmpty, sizeof(isEmpty));
	if (!isEmpty)
		do {
			read(fd, &res, sizeof(res));
			if(res.qryRes.job.jss == JOB_QUEUED)
				printQQR(&res);
		} while(!res.qryRes.islast);
}

/* RQR: Running Query Response */
void processRQR(int fd){
	serverResponse_t res;
	do {
		read(fd, &res, sizeof(res));
		if(res.qryRes.job.jss == JOB_RUNNING)
			printRQR(&res);
	} while(!res.qryRes.islast);
}

int main()
{
    int sockfd;
	
	//sockfd = setupClientSocket_UNIX("socketfile");
	sockfd = setupClientSocket_INET();

	commandRequest_t cmd;

	cmd = CMD_QUERY;
	write(sockfd, &cmd, sizeof(cmd));

	printHeader();
	processRQR(sockfd);
	processQQR(sockfd);
    close(sockfd);
    return 0;
}

