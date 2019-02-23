#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define NUM_THREADS 2
#define SIZE_QUE 50
#define USR_SIZE 16
#define ARG_SIZE 16
#define EXE_SIZE 40
#define SIZE_BUF 256
#define DATE_SIZE 25
#define MSG_SIZE 20
#define FNAME_SIZE 40
#define PORT 9930

typedef unsigned jobId_t;

typedef enum {
	CMD_SUBMIT,
	CMD_QUERY,
	CMD_DELETE,
} commandRequest_t;

typedef enum {
	JOB_QUEUED,
	JOB_RUNNING,
	JOB_KILLED,
} jobSubState_t; 

typedef enum {
	FALSE,
	TRUE,
} boolean_t;

typedef struct {
	char	user[USR_SIZE];
	char	exe[EXE_SIZE];
} exeFromUser_t; 

typedef struct {
	exeFromUser_t	its;
	unsigned		n_args;
} submitRequest_t;

typedef unsigned deleteRequest_t;

typedef struct {
	jobId_t 		id;
	jobSubState_t	jss;
	exeFromUser_t	its;
	unsigned		n_args;
	char 			**arg;	/* argumentos del ejecutable */
	time_t			submit;
	time_t			start;
} jobUnit_t;

typedef struct {
	struct tm	submit;	/* see man(3) localtime */
	struct tm	start;
} regTime_t;

typedef struct {
	unsigned	hours;
	unsigned	minutes;
	unsigned	seconds;
} runningTime_t;

typedef struct {
	jobId_t		id;
	char		msg[MSG_SIZE];
} submitResponse_t;

typedef struct {
	jobId_t			id;
	jobSubState_t	jss;
	exeFromUser_t	its;
	regTime_t		reg;
	runningTime_t	run;
} jobState_t;

typedef struct {
	jobState_t	job;
	boolean_t	islast;
} queryResponse_t;

typedef submitResponse_t deleteResponse_t;

typedef union {
	submitResponse_t	subRes;
	queryResponse_t		qryRes;
	deleteResponse_t	delRes;
} serverResponse_t;

typedef struct {
	unsigned 	h, t;
	jobUnit_t 	*pqueue;
	pthread_cond_t	sp;
	pthread_cond_t	el;
	pthread_mutex_t	mtx;
	boolean_t	isRunning;
} queue_t;	/* estructura de la cola */

typedef struct {
	unsigned	id;
	jobUnit_t	job;
	unsigned	cpid;	/* child pid */
} workerThread_t;


/* Algunas funciones que son comunes */

void error(const char *msg){
    perror(msg);
    exit(0);
}

int setupClientSocket_UNIX(char *path){
	int sockfd;
	struct sockaddr_un serv_addr;

	/* conexion al socketfile */
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, path);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
	return sockfd;	
}

int setupClientSocket_INET(){
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("error opening socket");

	server = gethostbyname("localhost");
	//server = gethostbyname("10.73.36.203");
	
	if (server == NULL){
		fprintf(stderr, "error, no such host\n");
		exit(0);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(PORT);
	
	/* now connect to the server */
	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		error("error connecting");

	return sockfd;
}
