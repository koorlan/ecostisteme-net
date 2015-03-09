#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>




#define MAX_ROOM 64

#define ALIASLEN 16
#define MSGBUFSIZE 256
#define LINEBUFF 256
#define PACKETBUFF 256


#define GAME_PORT 12345
#define GAME_GROUP "225.0.0.56"

typedef enum {SCAN_SEND, SCAN_REPLY, JOIN, LEAVE, U_POS } query_t;

typedef enum {DISCONNECTED,CONNECTING,CONNECTED } status_t;   //Disconnected =>ne recois/n'envoie rien //Connecting => recherche de partie //Connected => en jeu


typedef struct PACKETDATA {
  int protocolId;
  query_t query;
  int gameid;
  char sender[ALIASLEN];
  char buffer[PACKETBUFF];
} packetdata_t;



typedef struct ME {
  status_t status;
  char alias[ALIASLEN];
  int gameid;
} me_t;




#include "net.h"

const int ProtocolId = 0x38333631;  

int send_rate = 2;
int tab_room[MAX_ROOM];
me_t me;
int i_sock;
struct sockaddr_in i_addr;
int o_sock;
struct sockaddr_in o_addr;

queue_t received_queue;

pthread_mutex_t received_queue_mutex;
pthread_mutex_t me_mutex;
pthread_mutex_t send_rate_mutex;
pthread_mutex_t tab_room_mutex;

/*Lolfunc*/
void print_header(){
  /*Set windows size*/
  printf ("\e[8;25;95t");  
    /*Clear the Console*/
  printf("\033[H\033[2J");
            printf("\033[%sm","32");
         //  printf("\033[%sm","40");

  printf("\n");
  
   printf("\t\t                ,\"( \n");
printf("\t\t               ////\\                           _ \n");
printf("\t\t              (//////--,,,,,_____            ,\" \n");
printf("\t\t            _;\"\"\"----/////_______;,,        // \n");
printf("\t\t__________;\"o,-------------......\"\"\"\"\"`'-._/( \n");
printf("\t\t      \"\"'==._.__,;;;;\"\"\"           ____,.-.== \n");
printf("\t\t             \"-.:______,...;---\"\"/\"   \"    \\( \n");
printf("\t\t                 '-._      `-._(\"           \\ \n");
printf("\t\t                    '-._                    '._ \n");
printf("\n");
  printf("\033[%sm","0");
    printf("\033[%sm","36");
  printf("\033[%sm","40");
  printf("\t\t\t\t By Aubry Lisa & Colas Korlan\n");
  printf("\033[%sm","0");
}
//

int packet_check_protocol(packetdata_t packet);
void set_input_socket(int * sockfd, struct sockaddr_in *addr, char *group, int port);   //Pour les requetes qui arrivent
void set_output_socket(int * sockfd, struct sockaddr_in *addr,  char *group, int port);  //Pour envoyer des requetes
void *game_reciever();  //when we are ingame (another multicast adress)
void *network_reciever(); //when we are in lobby
void *dummy_input_game_generator(); //dummy bot for generate input while CONNECTED

void *network_sender();  //unused
void *process_packet_queue(); //analyze packet from every thread's reciever
void scan_send();

int main(int argc, char const *argv[])
{

 
 print_header();
	int err_ret,i;
	pthread_t th_dummy_input_game_generator;
  pthread_t th_network_reciever;
  pthread_t th_game_reciever;
  pthread_t th_process_packet_queue;
	char option[LINEBUFF];

  /*Initialize*/
  set_input_socket(&i_sock,&i_addr,GAME_GROUP,GAME_PORT);
  set_output_socket(&o_sock,&o_addr, GAME_GROUP, GAME_PORT);


  pthread_mutex_lock(&me_mutex);
  me.status = DISCONNECTED;
  me.gameid = 0;
  strcpy(me.alias,"Anon");
  pthread_mutex_unlock(&me_mutex);

  pthread_mutex_lock(&received_queue_mutex);
  QueueInit(&received_queue);
  pthread_mutex_unlock(&received_queue_mutex);
  
  

    while(fgets(option,sizeof(option),stdin) != 0) {
        option[strlen(option)-1]='\0';
        if(!strncmp(option, "quit", 4)) {
            break;
        }else if (!strncmp(option, "join", 4))
        {
          switch(me.status){
              case DISCONNECTED:
                printf("Starting NETW_reciever...");
                  if(pthread_create(&th_network_reciever, NULL, network_reciever, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  }else printf("\033[32mok\033[0m\n");

                  printf("Starting PACK_analyser...");
                  if(pthread_create(&th_process_packet_queue, NULL, process_packet_queue, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  } else printf("\033[32mok\033[0m\n"); 
              case CONNECTING:      
                 printf("Starting GAME_reciever...");
                  if(pthread_create(&th_game_reciever, NULL, game_reciever, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  } 
                  printf("\033[32mok\033[0m\n");
                  printf("Starting DUMMY_input...");
                  if(pthread_create(&th_dummy_input_game_generator, NULL, dummy_input_game_generator, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  } 
                  printf("\033[32mok\033[0m\n");

                  pthread_mutex_lock(&me_mutex);
                  me.gameid = atoi(&option[5]);
                  me.status = CONNECTED;
                  pthread_mutex_unlock(&me_mutex); 
                  break; 
              case CONNECTED:
                printf("\033[41m\033[37m\033[1mModule already loaded\033[0m\n");
                break;
              default:
                break;
            }
        }else if(!strncmp(option, "create", 6)) {

            switch(me.status){
              case DISCONNECTED:
                printf("Starting NETW_reciever...");
                  if(pthread_create(&th_network_reciever, NULL, network_reciever, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  }else printf("\033[32mok\033[0m\n");

                  printf("Starting PACK_analyser...");
                  if(pthread_create(&th_process_packet_queue, NULL, process_packet_queue, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  } else printf("\033[32mok\033[0m\n"); 
              case CONNECTING:      
                 printf("Starting GAME_reciever...");
                  if(pthread_create(&th_game_reciever, NULL, game_reciever, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  } 
                  printf("\033[32mok\033[0m\n");
                  printf("Starting DUMMY_input...");
                  if(pthread_create(&th_dummy_input_game_generator, NULL, dummy_input_game_generator, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  } 
                  printf("\033[32mok\033[0m\n");

                  //*Compute the maxgameid found on the network
                  scan_send();
                  sleep(3);
                  pthread_mutex_lock(&tab_room_mutex);
                  for (i = 1; i < MAX_ROOM; ++i) //sale methode le 1......
                  {
                   if ( !tab_room[i]){
                    pthread_mutex_unlock(&tab_room_mutex);
                    pthread_mutex_lock(&me_mutex);
                    me.gameid = i;
                    break;
                    }

                  }
                  me.status = CONNECTED;            
                  pthread_mutex_unlock(&me_mutex); 
                  break; 
              case CONNECTED:
                printf("\033[41m\033[37m\033[1mModule already loaded\033[0m\n");
                break;
              default:
                break;
            }
        }else if (!strncmp(option, "connect",7)){
          switch(me.status){
            case DISCONNECTED:
                  printf("Starting NETW_reciever...");
                  if(pthread_create(&th_network_reciever, NULL, network_reciever, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  }else printf("\033[32mok\033[0m\n");

                  printf("Starting PACK_analyser...");
                  if(pthread_create(&th_process_packet_queue, NULL, process_packet_queue, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  } else printf("\033[32mok\033[0m\n"); 
                  pthread_mutex_lock(&me_mutex);
                  me.status = CONNECTING;
                  pthread_mutex_unlock(&me_mutex);  
                  break;
            case CONNECTING:
            case CONNECTED:
                  printf("\033[41m\033[37m\033[1mModule already loaded\033[0m\n");
                  break;
            default:
                break;
          }
          
        }else if(!strncmp(option, "scan", 4)) {
          switch(me.status){
             case DISCONNECTED:
                  printf("Starting NETW_reciever...");
                  if(pthread_create(&th_network_reciever, NULL, network_reciever, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  }else printf("\033[32mok\033[0m\n");

                  printf("Starting PACK_analyser...");
                  if(pthread_create(&th_process_packet_queue, NULL, process_packet_queue, NULL) != 0) {
                     err_ret = errno;
                     fprintf(stderr, "pthread_create() failed...\n");
                     return err_ret;
                  } else printf("\033[32mok\033[0m\n"); 
                  pthread_mutex_lock(&me_mutex);
                  me.status = CONNECTING;
                  pthread_mutex_unlock(&me_mutex);  
                
            case CONNECTING:
              scan_send();
              sleep(3);
              for (i = 0; i < MAX_ROOM; ++i)
              {
                if ( tab_room[i])
                  printf("ROOM > %d -- USERS > %d\n",i,tab_room[i] );
              }
              break;
            case CONNECTED:
                  printf("\033[41m\033[37m\033[1mCan't scan while IN-GAME\033[0m\n");
                  break;
            default:
                break;
            }  
        }else if (!strncmp(option, "set", 3))
        {
          pthread_mutex_lock(&me_mutex);
          strcpy(me.alias,&option[4]);
          pthread_mutex_unlock(&me_mutex);
        
        }else fprintf(stderr, "Unknown option...\n");
    }
	return 0;
}
void set_input_socket(int * sockfd, struct sockaddr_in *addr,  char *group, int port){
	
     struct ip_mreq mreq;

     u_int yes=1;            /*** MODIFICATION TO ORIGINAL */

     /* create what looks like an ordinary UDP socket */
     if (( (*sockfd)=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
	  perror("socket");
	  exit(1);
     }


/**** MODIFICATION TO ORIGINAL */
    /* allow multiple sockets to use the same PORT number */
    if (setsockopt(*sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
       perror("Reusing ADDR failed");
       exit(1);
       }
/*** END OF MODIFICATION TO ORIGINAL */

     /* set up destination address */
     memset(addr,0,sizeof(*addr));
     (*addr).sin_family=AF_INET;
     
              
     
     (*addr).sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
     (*addr).sin_port=htons(port);
   
     /* bind to receive address */
     if (bind(*sockfd,(struct sockaddr *) addr,sizeof(*addr)) < 0) {
	  perror("bind");
	  exit(1);
     }

     int flags = fcntl(*sockfd, F_GETFL, 0);
  fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK);
     /* use setsockopt() to request that the kernel join a multicast group */
     mreq.imr_multiaddr.s_addr=inet_addr(group);
     mreq.imr_interface.s_addr=htonl(INADDR_ANY);
   if (setsockopt(*sockfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
	  perror("setsockopt");
	  exit(1);
     } 
}
void set_output_socket(int * sockfd, struct sockaddr_in *addr, char *group, int port){

      /* create what looks like an ordinary UDP socket */
     if ((*sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
      perror("socket");
      exit(1);
     }

     /* set up destination address */
     memset(addr,0,sizeof(*addr));
     (*addr).sin_family=AF_INET;
     (*addr).sin_addr.s_addr=inet_addr(group);
     (*addr).sin_port=htons(port);


}

int packet_check_protocol(packetdata_t packet){
  return (packet.protocolId == ProtocolId);
}




void *network_reciever(){

  packetdata_t packet;
  packetdata_t spacket;
  int i_addrlen=sizeof(i_addr);
  memset(&packet,0,sizeof(packetdata_t));
  memset(&spacket,0,sizeof(packetdata_t));   
  int maxfd = i_sock;
  fd_set static_rdset, rdset;

struct timeval static_timeout;
struct timeval timeout;
static_timeout.tv_sec = 0 ;
static_timeout.tv_usec =  1000;

FD_SET(i_sock, &static_rdset);


for (;;) {
  timeout = static_timeout;
  rdset = static_rdset;
  
   select(maxfd+1, &rdset, NULL, NULL, &timeout);
 if (FD_ISSET(maxfd, &rdset) ) {
    memset(&packet, 0, sizeof(packetdata_t));
    recvfrom(maxfd,(void*)&packet,sizeof(packetdata_t),0,(struct sockaddr *) &i_addr,(socklen_t *)&i_addrlen);
    if (!packet_check_protocol(packet)) continue;
    switch (me.status){
      case CONNECTED:
        if (!(packet.gameid == 0 || packet.gameid == me.gameid)) continue;
        break;
      default:
        break;
    }
    pthread_mutex_lock(&received_queue_mutex);
    QueuePut(&received_queue,packet);

   // printf("QueueSize : %d \n", received_queue.size  );
    pthread_mutex_unlock(&received_queue_mutex);
    }
       
  }


}

void *process_packet_queue(){
struct timespec tim, tim2;
   tim.tv_sec = 0;
   tim.tv_nsec = 500000;
  packetdata_t packet;
  packetdata_t spacket;
  memset(&packet,0,sizeof(packetdata_t));
  memset(&spacket,0,sizeof(packetdata_t));
  
  while(1){
    pthread_mutex_lock(&received_queue_mutex);
    if (!QueueGet(&received_queue,&packet))
    {
      pthread_mutex_unlock(&received_queue_mutex);
      switch (packet.query){

      case SCAN_SEND:
         
          printf("\033[%sm\033[%sm?>SCAN_SEND\033[%sm received\n","32","1","0");
          memset(&spacket,0,sizeof(packetdata_t));
          spacket.gameid = me.gameid;
          spacket.protocolId = ProtocolId;
          spacket.query = SCAN_REPLY;
          strcpy(spacket.sender,me.alias);
          sendto(o_sock,(void*)&spacket,sizeof(packetdata_t),0,(struct sockaddr *) &o_addr,sizeof(o_addr)); 
          printf("\033[%sm\033[%sm<?SCAN_REPLY\033[%sm generated\n","33","1","0");
          break;
      case SCAN_REPLY:
          switch (me.status){
              case DISCONNECTED:
              case CONNECTED:
                break;
              case CONNECTING:   //interesting case
                  pthread_mutex_lock(&tab_room_mutex);
                  tab_room[packet.gameid]++;
                  pthread_mutex_unlock(&tab_room_mutex);
        
                break;
              default:
                break;
              }
            break;    
          printf("\033[%sm\033[%sm?>SCAN_REPLY\033[%sm received\n","32","1","0");
            break;
      case U_POS:
            switch (me.status){
              case DISCONNECTED:
              case CONNECTING:
                break;
              case CONNECTED:
                printf("\033[%sm\033[%sm?>U_POS\033[%sm recieved : %s>>%s\n","32","1","0",packet.sender,packet.buffer);
                break;
              default:
                break;
              }
            break;      
      default:
            printf("\033[%sm\033[%sm?>ERROR\033[%sm received \n","31","1","0");
            break;
     }
     memset(&packet,0,sizeof(packetdata_t));
    }else{
      pthread_mutex_unlock(&received_queue_mutex);
      nanosleep(&tim , &tim2);
    }


  }

  return NULL;
}
        

void *game_reciever(){
  packetdata_t spacket;
 
  while(1){
     spacket.protocolId = ProtocolId;
  spacket.query = U_POS;
  spacket.gameid = 1;
  strcpy(spacket.sender,me.alias);
  strcpy(spacket.buffer,"JebougeOMG");
     //sendto(o_sock,(void*)&spacket,sizeof(packetdata_t),0,(struct sockaddr *) &o_addr,sizeof(o_addr));

     sleep(send_rate); 
  }
}

void *dummy_input_game_generator() {
  srand(time(NULL));
  int i;
  struct timespec tim, tim2;
   tim.tv_sec = 0;
   tim.tv_nsec = 5000000;
  packetdata_t spacket;
 
  while(1){
     spacket.protocolId = ProtocolId;
  spacket.query = U_POS;
  spacket.gameid = me.gameid;
  strcpy(spacket.sender,me.alias);
  for (i = 0; i < 60; ++i)
  {
    spacket.buffer[i] = 'A' + (random() % 26);
  }
  sendto(o_sock,(void*)&spacket,sizeof(packetdata_t),0,(struct sockaddr *) &o_addr,sizeof(o_addr));
  printf("\033[%sm\033[%sm<?U_POS\033[%sm generated\n","33","1","0"); 
    nanosleep(&tim , &tim2);
  }
}

void scan_send(){
/*reset tab_room*/
  pthread_mutex_lock(&tab_room_mutex);
  memset(tab_room,0,MAX_ROOM);
  pthread_mutex_unlock(&tab_room_mutex);
                /*Define Packet Content*/
  packetdata_t packet;
  memset(&packet, 0, sizeof(packetdata_t));
  packet.protocolId = ProtocolId;
  packet.query = SCAN_SEND;
  packet.gameid = me.gameid;
  strcpy(packet.sender,me.alias);
  if (sendto(o_sock,(void*)&packet,sizeof(packetdata_t),0,(struct sockaddr *) &o_addr,sizeof(o_addr)) < 0) {
   perror("sendto"); return ;
 } 
 printf("\033[%sm\033[%sm<?SCAN_SEND\033[%sm generated\n","33","1","0");
} 