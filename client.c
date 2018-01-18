#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
using namespace std;


#define ASK_LETTER 0
#define ASK_WORD 1
#define WRONG_ANSWER 2
#define RIGHT_ANSWER 3
#define YOUR_TURN 4
#define WELCOME 5
#define ROOMWAIT 6
#define START_GAME 7
#define YOU_WON 8

#define READ_BUFFER 256

extern int errno;

/* portul de conectare la server*/
int port;
char* readinput(size_t *size){

	char buffer;
	char* input;
	int sizebuff = 64;
	int i = 0;
	printf("Prezent in READINPUT, dupa declaratii\n");
	input = malloc(sizebuff);
	printf("Dupa malloc\n");
	do{
		printf("DO WHILE STARTS\n");
		buffer = getchar();
		printf("DUPA GETCHAR\n");

		if(buffer == '\n'){

			break;
		}

		if(i > sizebuff){

			printf("MARIM  BUFFER\n");
			sizebuff *= 2;
			input = realloc(input,sizebuff);
		}

		input[i] = buffer;
		i++;
		printf("FINAL DO WHILE\n");

	}while(1);
	printf("Am terminat de citit de la tastatura\n");

	input[i] = '\0';
	*size = i;
	printf("Am pus \\0 si acum ies din functie \n");
	return input;
}
int main (int argc, char *argv[])
{
  int sd;			
  struct sockaddr_in server;	
  int nr=0;
  char buf[10];

  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  port = atoi (argv[2]);

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }


  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons (port);
  
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }


    printf("Ne am conectat cu succes\n");

    int isValid = 1;
    int semnal;
    char litera;
    char word[READ_BUFFER];
   	int wordSize;
   
    while(isValid){

    	if(read(sd,&semnal,sizeof(int)) < 0){

    		perror ("[client]Eroare la write() spre server.\n");
      		exit(-1);
    	}

    	if(semnal == ASK_LETTER){

    		printf("Introdu o litera: \n");
    		scanf("%c",&litera);
    		if(write(sd,&litera,sizeof(char)) < 0){

    			perror ("[client]Eroare la write() spre server.\n");
      			exit(-1);
    		}

    	}

    	if(semnal == ASK_WORD){

    		
    		printf("Gimme THE WORD which starts with letter %c: \n", litera);
    		//word = readinput(&wordSize);
    		//fgets (word, READ_BUFFER, 0);      /* read in a line */
    		char i = getchar();
    		wordSize = 1;
    		
    		/*if(write(sd,&wordSize,sizeof(int)) < 0){

                perror ("[client]Eroare la write() spre client.\n");
                exit(-1);
              }*/
             if(write(sd,&i,wordSize) < 0){

                perror ("[client]Eroare la write() spre client.\n");
                exit(-1);
              }
		}

		if(semnal == WRONG_ANSWER){

			printf("You got the wrong word, GAME OVER\n");
			isValid = 0;
		}

		if(semnal == RIGHT_ANSWER){

			printf("Congratulation by Post Malone, whait for your nest turn\n");
		}

		if(semnal == YOUR_TURN){

			printf("Your turn is on!\n");
		}
		if(semnal == WELCOME){

			printf("Welcome to the game!\n");
		}
		if(semnal == ROOMWAIT){

			printf("Wait till the room is fool\n");
		}
		if(semnal == START_GAME){

			printf("So the room is full, START\n");
		}
		if(semnal == YOU_WON){

			printf("This is a miracle!!!!\n");
		}
		semnal = -1;
    }

    	
 /* printf ("[client]Introduceti un numar: ");
  fflush (stdout);
  read (0, buf, sizeof(buf));
  nr=atoi(buf);*/
  //scanf("%d",&nr);
  
  //printf("[client] Am citit %d\n",nr);

  /* trimiterea mesajului la server */
  /*if (write (sd,&nr,sizeof(int)) <= 0)
    {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }*/

  /* citirea raspunsului dat de server 
     (apel blocant pina cind serverul raspunde) */
  /*if (read (sd, &nr,sizeof(int)) < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
    }*/
  /* afisam mesajul primit */
  //printf ("[client]Mesajul primit este: %d\n", nr);

  /* inchidem conexiunea, am terminat */
  close (sd);
}