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
#define ROOMWAIT 6
#define START_GAME 7
#define YOU_WON 8
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

#define PLAYERS_PER_ROOM 3
#define SIZE_OF_ROOM_POOL 20
typedef struct {
  
  int playerTurn; 
  int turn;
  int playersId[PLAYERS_PER_ROOM];
  int numarJucatori;

}Room;

Room roomPool[SIZE_OF_ROOM_POOL];

void initRoomPool(Room *pool){

  int index;
  for(index = 0; index < SIZE_OF_ROOM_POOL; index++){

    pool[index].playerTurn = 0;
    pool[index].turn = 0;
    pool[index].numarJucatori = 0;
    int j;
    for(j=0;j < PLAYERS_PER_ROOM;j++){
      pool[index].playersId[j] = 0;
    }
  }
}

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

void setupDictionar(char *dictionar,int *dimensiuneDic,const char *fisier){

dictionar = 0;
int size;
FILE * dicFile = fopen (fisier, "rb");

if (dicFile)
{
  fseek (dicFile, 0, SEEK_END);
  size = ftell (dicFile);
  fseek (dicFile, 0, SEEK_SET);
  dictionar = malloc (size);
  if (dictionar)
  {
    fread (dictionar, 1, size, dicFile);
  }
  dictionar[size] = '\0';
  if (dictionar[size] == '\0' )printf("AM NULL CHARAC %c %c\n",dictionar[size-2],dictionar[size-1]);
  else printf("NU STIU CE I CU MINE\n");
  
  *dimensiuneDic = size;
  fclose (dicFile);
}
}


void raspunde(int cl,int idThread,int camera);
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
  nthreads = 3;
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
    int i = (int)arg;
            
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

        roomPool[camera].playerTurn = threadsList[i];
      }
      roomPool[camera].playersId[roomPool[camera].numarJucatori] = threadsList[i];
      roomPool[camera].numarJucatori++;
      if(connections % 3 == 0){

        camera++;
      }
      pthread_mutex_unlock(&mlock);
      raspunde(client,i,camera); //procesarea cererii
      /* am terminat cu acest client, inchidem conexiunea */
      close (client);     
      
}
void sendSignal(int client,int signal){

  if(write(client,&signal,sizeof(int)) < 0){

    perror ("[client]Eroare la write() spre client.\n");
    exit(-1);
  }
}
char* readWordFromClient(int client){


  printf("SUNT IN READ WORD FROM CLIENT\n");
  int size;
  char *word;
  if(read(client,&size,sizeof(int)) < 0){

      perror ("[client]Eroare la write() spre client.\n");
      exit(-1);
  }

  if(size == 0){

    printf("NU am citit nimic\n");
    exit(-1);
  }
  printf("AM CITIT SIZE WORD %d\n", size);
  word = malloc(size);
  if(read(client,word,size) < 0){

      perror ("[client]Eroare la write() spre client.\n");
      exit(-1);
  }
  printf("WORD %s\n", word);
  word[size] = '\0';
  if (word[size] == '\0' )printf("AM NULL CHARAC %c %c\n", word[size-2],word[size-1]);
  return word;

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


  printf("Am intrat in validate word\n");
  if(dictionarSize < 1){

    printf("Nu exista dictionar\n");
    exit(-1);
  }
  if(word[0] != primalit){

    printf("Litere diferite\n");
    return 0;
  }

  printf("Verificam subsir\n");
  if(strstr(dictionar,word) != NULL){

      printf("Its a match\n");
      return 1;
    
  }
  return 0;
}
void raspunde(int cl,int idThread,int room)
{
        int isValid = 1;
        int lungimeCuvant;
        int index;


        char litera;
        char *cuvant;

        int camera = room;
        
        sendSignal(cl,WELCOME);
        printf("[THREAD %d] WELCOME\n", idThread);
        
        sendSignal(cl,ROOMWAIT);
        printf("[THREAD %d] ROOMWAIT\n", idThread);
        
        sendSignal(cl,START_GAME);
        printf("[THREAD %d] START_GAME\n", idThread);

        do{//while he is in the game

          if(threadsList[idThread] == roomPool[camera].playerTurn){

            sendSignal(cl,YOUR_TURN);
            printf("[THREAD %d] YOUR_TURN\n", idThread);
            if(roomPool[camera].turn == 0){

              sendSignal(cl,ASK_LETTER);
              printf("[THREAD %d] ASK_LETTER\n", idThread);
              litera = readCharFormClient(cl);

            }

            sendSignal(cl,ASK_WORD);
            printf("[THREAD %d] ASK_WORD\n", idThread);
            cuvant = readWordFromClient(cl);

            printf("[THREAD %d] Am citit cuvantul %s\n", idThread,cuvant);

            isValid = validateWord(cuvant,litera);
            printf("[THREAD %d] Am apelat validateWord\n", idThread);

            if(isValid) {
              sendSignal(cl,RIGHT_ANSWER);
              printf("[THREAD %d] RIGHT_ANSWER, adica e valid\n", idThread);

              if(roomPool[camera].numarJucatori == 1){

                sendSignal(cl,YOU_WON);
                printf("[THREAD %d] YOU_WON, un if cam sumbru\n", idThread);
              }
              index = roomPool[camera].turn % roomPool[camera].numarJucatori;
              printf("[THREAD %d] calc INDEX schimbare tura\n", idThread);
              while(roomPool[camera].playersId[index] == 0){

                index = (index + 1) % roomPool[camera].numarJucatori;
              }
              printf("[THREAD %d] WHILE index valid\n", idThread);
              
              pthread_mutex_lock(&mlock);
              printf("[THREAD %d] Sunt in mutex\n", idThread);
              roomPool[camera].playerTurn = roomPool[camera].playersId[index];
              pthread_mutex_unlock(&mlock);
              printf("[THREAD %d] MUTEX OUT, SCHIMBAT PLAYER\n", idThread);

            }
            else{

              sendSignal(cl,WRONG_ANSWER);
              printf("[THREAD %d] WRONG_ANSWER\n", idThread);
            }

            pthread_mutex_lock(&mlock);
            roomPool[camera].turn++;
            printf("[THREAD %d] SCHIMBAT TURA IN MUTEX\n", idThread);
            pthread_mutex_unlock(&mlock);
          }
          
        }while(isValid);
        
        pthread_mutex_lock(&mlock);
        roomPool[camera].numarJucatori--;
        roomPool[camera].playersId[roomPool[camera].playerTurn] = 0;
        pthread_mutex_unlock(&mlock);
        printf("[THREAD %d] Dupa mutex, schimbat tura\n", idThread);
        index = roomPool[camera].turn % roomPool[camera].numarJucatori;
        while(roomPool[camera].playersId[index] == 0){

          index = (index + 1) % roomPool[camera].numarJucatori;
        }
              
        pthread_mutex_lock(&mlock);
        roomPool[camera].playerTurn = roomPool[camera].playersId[index];
        pthread_mutex_unlock(&mlock);
}