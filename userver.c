#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>

typedef struct {
	char user[20];
	char exe[20];
	unsigned n_args;
} CmdReq_t;


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd;
     char buffer[256];
     struct sockaddr_un serv_addr;
     int n;
     sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     serv_addr.sun_family = AF_UNIX;
     strcpy(serv_addr.sun_path, "tmp");
     unlink(argv[1]);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     newsockfd = accept(sockfd, NULL, NULL);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);

	CmdReq_t cmd;
     n = read(newsockfd, &cmd, sizeof(cmd));
     if (n < 0) error("ERROR reading from socket");
     printf("Here is the message: %s!\n", cmd.user);
     printf("Here is the message: %s!\n", cmd.exe);
     printf("Here is the message: %d!\n", cmd.n_args);

     n = write(newsockfd,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");
     close(newsockfd);
     close(sockfd);
     return 0; 
}
