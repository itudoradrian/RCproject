#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#define PORT 2025

int setSocket(int domain,int type,int protocol){

	int err = socket(domain,type,protocol);
	if(err < 0){

		perror("Eroare socket creation");
		exit(-1);
	}
	return err;
}
void setSockaddrin(int family,int port,struct sockaddr_in *str){

	str->sin_family = family;
	str->sin_port = htons(port);
	str->sin_addr.s_addr = htonl(INADDR_ANY);

}
int main(int argc, char const *argv[])
{
	struct sockaddr_in server;	// structura folosita de server
    struct sockaddr_in from;	
    int socketDesc;			//descriptorul de socket 
    pid_t child;

    socketDesc = setSocket(AF_INET, SOCK_STREAM, 0);
    bzero (&server, sizeof (server));
  	bzero (&from, sizeof (from));

  	setSockaddrin(AF_INET,PORT,&server);

  	if(bind(socketDesc,(struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

    if (listen (socketDesc, 5) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }

    //am setat serverul pana acum
    //tratam conexiunile

    while(1){

    	int descDeClient;
    	int lengthFrom = sizeof(from);
    	printf ("[server]Asteptam la portul %d...\n",PORT);
      	fflush (stdout);

    	descDeClient = accept (socketDesc,(struct sockaddr *)&from,&lengthFrom);
    	if (descDeClient < 0)
	       {
	         perror ("[server]Eroare la accept().\n");
	         continue;
	       }
    	printf("Am primit un client\n");

    	child = fork();
    	if(child <= -1){

      	perror("Eroare la fork");
      	exit(-1);
      	}

      	if(child == 0){

      		close(socketDesc);
      		printf("Hello from the child\n");
      		close (descDeClient);
      	}

      	close(descDeClient);
    	while(waitpid(-1,NULL, WNOHANG) > 0);
    }
	
}