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

#define CONFIGURATION "nrOfPlayers.txt"
#define DICTIONAR "dictionar.txt"
#define ASK_LETTER 0
#define ASK_WORD 1
#define WRONG_ANSWER 2
#define RIGHT_ANSWER 3
#define YOUR_TURN 4
#define WELCOME 5
#define MAX_CONNECTIONS 100
#define SENDING_ROOM_NUMBER 5


extern int errno;
int port;
int sd; //descriptorul de socket de ascultare
int nthreads;//numarul de threaduri
int connections = 0;
int isLetterChosen = 0, isGameStarted = 0, numberOfRooms = 1;
int dictionarSize;
int camera = 1;



char *dictionar;

pthread_t* threadsList;
pthread_mutex_t mlock=PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  
  int playerTurn = 0;
  int turn = 0;
  int playersId[3];
  int numarJucatori = 0;

}Room;

Room roomPool[20];

int readFromConfiguration(const char *fileName){

  int filedesc;
  filedesc = open(fileName,O_RDONLY);
  if(filedesc < 0){

    printf("Eroare la deschidere fisier: %s\n", fileName);
    exit(-1);
  }

  int numar;
  if (read(filedesc,&numar,sizeof(int)) < 0)
  {
    printf("Eroare la read din fisier %s\n",fileName);
    exit(-1);
  }

  close(filedesc);
  return numar;
}

void setupDictionar(char *dictionar,int *size,const char *fisier){

dictionar = 0;
FILE * dicFile = fopen (fisier, "rb");

if (dicFile)
{
  fseek (dicFile, 0, SEEK_END);
  *size = ftell (dicFile);
  fseek (dicFile, 0, SEEK_SET);
  dictionar = malloc (*size);
  if (dictionar)
  {
    fread (dictionar, 1, *size, dicFile);
  }
  fclose (dicFile);
}
}


void raspunde(int cl,int idThread);
int main (int argc, char *argv[])
{
  
  struct sockaddr_in server;
  struct sockaddr_in from;   
  void threadCreate(int);
  int length = sizeof (from);
  bzero (&from, sizeof (from));
  
  if(argc<2){
        
      fprintf(stderr,"Eroare: Primul argument este portul...");
      exit(1);
   }
  port = atoi(argv[1]);
  
  setupDictionar(dictionar,&dictionarSize,DICTIONAR);
  nthreads = readFromConfiguration(CONFIGURATION);
  if(nthreads < 2){
        
    fprintf(stderr,"Eroare: Prea putin playeri...");
    exit(1);
  }
  threadsList = calloc(sizeof(pthread_t),nthreads);
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  
  bzero (&server, sizeof (server));
  server.sin_family = AF_INET;  
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (port);
    
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }

  //Pana aici sigur functioneaza

  printf("Nr de utilizatori %d \n", nthreads); fflush(stdout);
  int i;
  for(i=0; i<nthreads;i++) threadCreate(i);

  for ( ; ; )
    {
        printf ("[server]Asteptam la portul %d...\n",port);
        pause();        
     }//while

     return 0;


};
void threadCreate(int i)
{
  void *treat(void *);
  
  pthread_create(&threadsList[i],NULL,&treat,(void*)i);
  return; /* threadul principal returneaza */
}
void *treat(void * arg)
{   
    int client;
            
    struct sockaddr_in from; 
      bzero (&from, sizeof (from));
          

    printf ("[thread]- %d - pornit...\n", (int) arg);fflush(stdout);

    
      int length = sizeof (from);
      pthread_mutex_lock(&mlock);
      //printf("Thread %d trezit\n",(int)arg);
      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
        {
         perror ("[thread]Eroare la accept().\n");          
        }
      pthread_mutex_unlock(&mlock);

      pthread_mutex_lock(&mlock);
      connections++;
      
      if(roomPool[camera].playerTurn == 0){

        roomPool[camera].playerTurn = (int)arg;
      }
      roomPool[camera].playersId[roomPool[camera].numarJucatori] = (int)arg;
      roomPool[camera].numarJucatori++;
      if(connections % 3 == 0){

        camera++;
      }
      pthread_mutex_unlock(&mlock);
      raspunde(client,(int)arg,camera); //procesarea cererii
      /* am terminat cu acest client, inchidem conexiunea */
      close (client);     
      
}
void sendSignal(int client,int signal){

  if(write(client,&signal,sizeof(int)) < 0){

    perror ("[client]Eroare la write() spre client.\n");
    exit(-1);
  }
}
int readWordFromClient(int client,char *word){

  int size;
  if(read(client,&size,sizeof(int)) < 0){

      perror ("[client]Eroare la write() spre client.\n");
      exit(-1);
  }
  word = malloc(size);
  if(read(client,&word,size) < 0){

      perror ("[client]Eroare la write() spre client.\n");
      exit(-1);
  }
  word[size] = '\0';
  return size;

}
char readCharFormClient(int client){

  char caracter;
  if(read(client,&caracter,sizeof(char)) < 0){

      perror ("[client]Eroare la write() spre client.\n");
      exit(-1);
  }
  return caracter;
}
int validateWord(char* word,char primalit){

  if(dictionarSize < 1){

    printf("Nu exista dictionar\n");
    exit(-1);
  }
  if(word[0] != primalit){

    return 0;
  }
  for (int i = 0; i < dictionarSize; i++)
  {
    if(strstr(dictionar,word) != NULL){

      return 1;
    }
  }
  return 0;
}
void raspunde(int cl,int idThread,int room)
{
        int isValid = 1;
        int lungimeCuvant;

        char litera;
        char *cuvant;

        int camera = room;
        

        sendSignal(cl,WELCOME);
        while(isValid){//while he is in the game

          if(idThread == playerTurn){

            sendSignal(cl,YOUR_TURN);
            if(turn == 0){

              sendSignal(cl,ASK_LETTER);
              litera = readCharFormClient(cl);

            }

            sendSignal(cl,ASK_WORD);
            lungimeCuvant = readWordFromClient(cl,cuvant);

            isValid = validateWord(cuvant,litera);

            if(isValid) {
              sendSignal(cl,RIGHT_ANSWER);
              playerTurn++;
            }
          }
          turn++;
        }

        sendSignal(WRONG_ANSWER);
}