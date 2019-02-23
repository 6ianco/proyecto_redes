#include "mylib.h"

/* defino variables globales */
unsigned COUNT = 0;
queue_t Q;
workerThread_t wt[NUM_THREADS];

/* funcion para crear la cola */
void createQueue(queue_t *pq){
	pq->h = 0;
	pq->t = 0;
	pq->isRunning = TRUE;
	pq->pqueue = (jobUnit_t *) malloc(SIZE_QUE*sizeof(jobUnit_t));
	pthread_mutex_init(&pq->mtx, NULL);
	pthread_cond_init(&pq->sp, NULL);
	pthread_cond_init(&pq->el, NULL);
}

/* funcion para poner un job unit en la cola */
void queuePut(queue_t *pq, jobUnit_t *pj){
	pthread_mutex_lock(&pq->mtx);
	while((pq->t - pq->h) == SIZE_QUE)
		pthread_cond_wait(&pq->sp, &pq->mtx);
	time(&pj->submit);
	pj->jss = JOB_QUEUED; /* marcado como encolado */
	pq->pqueue[pq->t % SIZE_QUE] = *pj;
	pq->t++;
	printf(">> job->queue @ %s\n", asctime(localtime(&pj->submit)));		
	pthread_mutex_unlock(&pq->mtx);
	pthread_cond_broadcast(&pq->el);
}

/* funcion para sacar un job unit de la cola */
jobUnit_t queueGet(queue_t *pq){
	jobUnit_t job;
	pthread_mutex_lock(&pq->mtx);
	while((pq->t - pq->h) <= 0)
		pthread_cond_wait(&pq->el, &pq->mtx);
	job = pq->pqueue[pq->h % SIZE_QUE];
	pq->h++;
	pthread_mutex_unlock(&pq->mtx);
	pthread_cond_broadcast(&pq->sp);
	
	/* solo sale si esta marcado como encolado,
     * sino busco el siguiente */
	if(job.jss == JOB_QUEUED)
		return job;
	else
		return queueGet(&Q);
}

/* funcion para crear el socket tipo UNIX */
int setupServerSocket_UNIX(char *path){
	int sockfd;
	struct sockaddr_un serv_addr;
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, path);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	return sockfd;
}

int setupServerSocket_INET(){
	int sockfd;
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0)
		error("Error connecting socket");

	/* Initialize socket structure */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	/* now bind the host addres using bind() call */
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		error("error on binding");

	return sockfd;
}
	


/* funcion que calcula el tiempo que lleva corriendo un job unit
 * requirido por ./qstat */
runningTime_t processRunningTime(time_t *regStart){
	runningTime_t t;
	if(regStart){
		time_t	ahora;
		unsigned delta;
		time(&ahora);
		delta = ahora - *regStart;
		t.hours = delta/3600;
		t.minutes = (delta - t.hours*3600)/60;
		t.seconds = delta - t.hours*3600 - t.minutes*60;
	} else {
		t.hours = 0;
		t.minutes = 0;
		t.seconds = 0;
	}
	return t;
}

/* requerido por ./qstat
 * funcion que le dice al cliente que job units estan
 * estan en la cola esperando */
void processQueueQuery(int fd){
	serverResponse_t res;
	jobUnit_t *pj;
	unsigned i;

	pthread_mutex_lock(&Q.mtx);
	unsigned start = Q.h;
	unsigned end = Q.t;
	boolean_t isEmpty = FALSE; 
	if ((end-start) % SIZE_QUE == 0)
		isEmpty = TRUE;
	write(fd, &isEmpty, sizeof(isEmpty));
	for (i=start; i<end; i++){
		pj = &Q.pqueue[i % SIZE_QUE];
		res.qryRes.job.id = pj->id;
		res.qryRes.job.jss = pj->jss;
		res.qryRes.job.its = pj->its;
		res.qryRes.job.reg.submit = *localtime(&pj->submit);
		//res.qryRes.job.reg.start = *localtime(NULL);
		res.qryRes.job.run = processRunningTime(NULL);
		res.qryRes.islast = (i+1 == end) ? TRUE : FALSE;
		write(fd, &res, sizeof(res));
	}
	pthread_mutex_unlock(&Q.mtx);
}

/* requerido por ./qstat
 * funcion que le dice al cliente que job units estan
 * corriendo */
void processRunningQuery(int fd){
	serverResponse_t res;
	jobUnit_t *pj;
	unsigned i;
	for(i=0; i< NUM_THREADS; i++){
		pj = &wt[i].job;
		res.qryRes.job.id = pj->id;
		res.qryRes.job.jss = pj->jss;
		res.qryRes.job.its = pj->its;
		res.qryRes.job.reg.submit = *localtime(&pj->submit);
		res.qryRes.job.reg.start = *localtime(&pj->start);
		res.qryRes.job.run = processRunningTime(&pj->start);
		res.qryRes.islast = (i+1 == NUM_THREADS)? TRUE : FALSE;
		write(fd, &res, sizeof(res));
	}
}

/* requerido por ./qstat
 * responde al cliente info de los job units */
void processQuery(int fd){
	processRunningQuery(fd);
	processQueueQuery(fd);
}

/* requerido por ./qsub
 * responde al cliente con mensaje de confirmacion */
void submitResponse(jobId_t id, int fd){
	serverResponse_t res;
	res.subRes.id = id;
	strcpy(res.subRes.msg, "it has been submitted!");
	write(fd, &res, sizeof(res));
}

/* libera memoria alocada para los argumentos del ejecutable de un job */
void cleanMemory(jobUnit_t *pj){
	int i;
	for	(i=0; i < pj->n_args; i++)
		free((void *) pj->arg[i]);

	free((void *) pj->arg);
}

/* requerido por ./qsub
 * procesa la peticion del cliente al enviar un job unit */
void processSubmit(int fd){
	submitRequest_t req;
	jobUnit_t job;
	int i;
	
	/* leo el submitRequest */
	read(fd, &req, sizeof(req));

	/* proceso el submit, lo transformo en un unitJob */
	job.id = ++COUNT;
	job.its = req.its;
	job.n_args = req.n_args;

	/* proceso los argumentos del ejecutable
	 * n_args contiene el nombre del ejecutable
	 * y agrego un puntero para NULL, requerido por execvp
	 */
	job.arg = (char **)malloc((job.n_args + 1) * sizeof(char *));
	size_t argSz;
	for(i=0; i< job.n_args; i++){
		read(fd, &argSz, sizeof(argSz));
		job.arg[i] = (char *)malloc(argSz+1);
		read(fd, job.arg[i], argSz);
		job.arg[i][argSz] = 0;
	}
	/* ultimo puntero tiene que apuntat a NULL,
	 * ver man(3) execvp
	 */
	job.arg[job.n_args] = NULL;

	/* Ahora que tengo el UnitJob, lo mando a la cola */
	queuePut(&Q, &job);

	/* Ahora devuelvo un mensaje al cliente */
	submitResponse(job.id, fd);
}

/* requerido por ./qdel
 * elimina un job que se encuentra en la cola o si esta corriendo */
void processDelete(int fd){
	unsigned i, start, end;
	jobUnit_t *pj;
	deleteRequest_t jobId;
	serverResponse_t res;

	/* leo el jobId a eliminar */
	read(fd, &jobId, sizeof(jobId));
	res.delRes.id = jobId;

	/* supongo que inicialmente no lo voy a encontrar,
     * (ej. cuando el cliente da un jobId invalido)
     * luego sobre-escribo el mensaje si lo encuentro */
	strcpy(res.delRes.msg, "no fue encontrado");

	/* primero me fijo si dicho job ya esta corriendo */
	// LOCK?
	for(i=0; i<NUM_THREADS; i++){
		if ((wt[i].job.id == res.delRes.id)&&(wt[i].job.jss == JOB_RUNNING)){
			if ( kill(wt[i].cpid, SIGTERM) == 0) {
				strcpy(res.delRes.msg, "ha sido eliminado");
				/* respondo al cliente */
				write(fd, &res, sizeof(res));	
			}
			else
				error("kill");
			break;
		}
	}

	/* Si no estaba corriendo me fijo si esta en la cola */
	start = Q.h;
	end = Q.t;
	for(i=start; i<end; i++){
		pj = &Q.pqueue[i % SIZE_QUE];
		if((pj->id == res.delRes.id)&&(pj->jss == JOB_QUEUED)){
			/* si lo encuentro lo marco como borrado y libero memoria */
			pj->jss = JOB_KILLED;
			cleanMemory(pj);
			strcpy(res.delRes.msg, "ha sido eliminado");
			printf(">> job %d removed from queue\n\n", pj->id);
			break;
		}
	}
	/* respondo al cliente */
	write(fd, &res, sizeof(res));	
}

/* funcion que atiende la peticion del cliente */
void attendClient(int fd){
	commandRequest_t cmd;
	int sockfd = accept(fd, NULL, NULL);

	read(sockfd, &cmd, sizeof(cmd));
	
	switch(cmd){
		case CMD_SUBMIT:
			processSubmit(sockfd);
			break;
		case CMD_QUERY:
			processQuery(sockfd);
			break;
		case CMD_DELETE:
			processDelete(sockfd);
	}
//	close(fd);
}

/* funcion que procesa cada thread
 * cada thread saca un job unit de la cola y hace un fork,
 * el hijo ejecuta el exe con los argumentos,
 * el thread espera a que termine el hijo */
void* assignWork2thread(void* arg){
	unsigned pid;
	int status;
	char screen[FNAME_SIZE];
	workerThread_t *pwt;

	pwt = (workerThread_t *) arg;

	while(Q.isRunning){
		pwt->job = queueGet(&Q);
		time(&pwt->job.start);
		pwt->job.jss = JOB_RUNNING;

		pid = fork();
		
		switch(pid){
			case 0:
				/* child process */
				/* Creo un archivo llamado screen para redirigir stdout
				 * y stderr */
				sprintf(screen, "%d", pwt->job.id);
				strcat(screen, ".");
				strcat(screen, pwt->job.its.user);
				strcat(screen, ".");
				strcat(screen, pwt->job.its.exe);
				int newfd = open(screen, O_RDWR|O_CREAT|O_APPEND, 0600);
				dup2(newfd, STDOUT_FILENO); 
				dup2(newfd, STDERR_FILENO); 
				close(newfd);

				sleep(20);
				printf("A punto de correr %s\n", pwt->job.its.exe);
				execvp(pwt->job.its.exe, pwt->job.arg);
				error(pwt->job.its.exe);
				return NULL;
			
			case -1:
				error("fork");
				break;

			default:
				pwt->cpid = pid;
				pid = waitpid(pid, &status, 0);
				assert(pid != -1);
				if (WIFSIGNALED(status))
					printf(">> job %d killed\n\n", pwt->job.id);
				else
					printf(">> job %d finished normally\n\n", pwt->job.id);
				/* lo marco porque ya termino y libero memoria */
				pwt->job.jss = JOB_KILLED;
				cleanMemory(&pwt->job);
				break;
		}
	}
	return NULL;
}

/* funcion principal */
int main(void)
{
    int sockfd, i;

	unlink("socketfile");
	//sockfd = setupServerSocket_UNIX("socketfile");
	sockfd = setupServerSocket_INET();
	createQueue(&Q);
	
	/* Creo pool de threads para que ejecuten los progrmas */
	pthread_t thread[NUM_THREADS];
	for(i=0; i<NUM_THREADS; i++){
		wt[i].id = i;
		pthread_create(&thread[i], NULL, assignWork2thread, (char *) &wt[i]);
	}

    listen(sockfd,5);
	while(1){
		attendClient(sockfd);
	}
	
    return 0; 
}
