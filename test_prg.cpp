#include "goldchase.h"
#include "Map.h"
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <fstream>
#include <unistd.h>
#include <string>
#include <semaphore.h>
#include <signal.h>
#include <mqueue.h>
#include <stdlib.h>
#include "fancyRW.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> //for READ/write
#include <string.h> //for memset
#include <stdio.h> //for fprintf, stderr, etc.
#include <stdlib.h>
#include <sstream>


#define MAPSIZE (sizeof(int))
using namespace std;
struct GameBoard
{
    int rows; //4 bytes
    int columns; //4 bytes
    unsigned char players;
    pid_t pid[5];
    int daemonID;
    unsigned char map[0];
};


int sockfd;
int new_sockfd;
int status;
mqd_t queueArray[5];
int fd = -99;

GameBoard* goldmap;
GameBoard* goldmapServer;
GameBoard* goldmapClient;
unsigned char* local;
int rows_in_map = 1;
int columns_in_map =1;
int pipefd[2];
sem_t* mysemaphore;
bool forcequit=false;
Map *mapp;
unsigned char map_daemon;


void handle_interrupt(int)
{
  mapp->drawMap();
  sem_wait(mysemaphore);
  sem_post(mysemaphore);
}
mqd_t readqueue_fd; //message queue file descriptor
string mq_name;

void clean_up(int)
{
  forcequit=true;
  sem_wait(mysemaphore);
  sem_post(mysemaphore);
}

void serverStartup();
void clientStartup(string);


void read_message(int)
{
  struct sigevent mq_notification_event;
  mq_notification_event.sigev_notify=SIGEV_SIGNAL;
  mq_notification_event.sigev_signo=SIGUSR2;
  mq_notify(readqueue_fd, &mq_notification_event);

  int err;
  char msg[121];
  memset(msg, 0, 121);//set all characters to '\0'
  while((err=mq_receive(readqueue_fd, msg, 120, NULL))!=-1)
  {
    mapp->postNotice(msg);
    memset(msg, 0, 121);//set all characters to '\0'
  }
  if(errno!=EAGAIN)
  {
    perror("mq_receive");
    exit(1);
  }
}

void write_message(int sender,int to,string msg)
{
  if(sender == 1)
    msg = "Player 1 :" + msg;
  if(sender == 2)
    msg = "Player 2 :" + msg;
  if(sender == 3)
    msg = "Player 3 :" + msg;
  if(sender == 4)
    msg = "Player 4 :" + msg;
  if(sender == 5)
    msg = "Player 5 :" + msg;

  mqd_t writequeue_fd; //message queue file descriptor
  string mq;
  if(to == 0)
    mq="/myplra1";
  else if(to == 1)
    mq="/myplra2";
  else if(to == 2)
    mq="/myplra3";
  else if(to == 3)
    mq="/myplra4";
  else if(to == 4)
    mq="/myplra5";
  if((writequeue_fd=mq_open(mq.c_str(), O_WRONLY|O_NONBLOCK))==-1)
  {
    perror("mq_open");
    exit(1);
  }
  char message_text[121];
  memset(message_text, 0, 121);
  strncpy(message_text, msg.c_str(), 120);

  if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
  {
    perror("mq_send");
    exit(1);
  }
  mq_close(writequeue_fd);
}



int main(int argc, char * argv[])
{
  if(argc > 1)
  {
    string ip = argv[1];
    clientStartup(ip);
 }

    struct sigaction action_jackson;
    action_jackson.sa_handler=handle_interrupt;
    sigemptyset(&action_jackson.sa_mask);
    action_jackson.sa_flags=0;
    action_jackson.sa_restorer=NULL;

    sigaction(SIGUSR1, &action_jackson, NULL);


    struct sigaction exit_handler;
    exit_handler.sa_handler=clean_up;
    sigemptyset(&exit_handler.sa_mask);
    exit_handler.sa_flags=0;
    sigaction(SIGINT, &exit_handler, NULL);
    sigaction(SIGTERM,&exit_handler,NULL);

    struct sigaction action_to_take;
    action_to_take.sa_handler=read_message;
    sigemptyset(&action_to_take.sa_mask);
    action_to_take.sa_flags=0;
    sigaction(SIGUSR2, &action_to_take, NULL);

    struct mq_attr mq_attributes;
    mq_attributes.mq_flags=0;
    mq_attributes.mq_maxmsg=10;
    mq_attributes.mq_msgsize=120;

	FILE* fp;
	int i;

	int fd;int curr_pos=0;

  char map[0];
    unsigned char curr_player;
	srand (time(NULL));
		int random=0;
		int x = 0;
		int j=2;



      mysemaphore= sem_open("/AAgoldchase", O_CREAT|O_EXCL, S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR, 1);


 	//sem_unlink("/AAgoldchase");
	//shm_unlink("/shm_obj1");


 if(mysemaphore == SEM_FAILED)
    {
        if(errno!=EEXIST)
         {
         	perror("Semaphore already exists");
            exit(1);
         }

    }

    //mq_close(readqueue_fd);
    //mq_unlink(mq_name.c_str());
	//	sem_unlink("/AAgoldchase");
		//shm_unlink("/shm_obj1");

	//First Player
	if(mysemaphore!=SEM_FAILED)
	{
		sem_wait(mysemaphore);
		int result;
//		char gold;
//		string s;
		fd=shm_open("/shm_obj1", O_CREAT|O_RDWR,S_IWUSR|S_IRUSR);
		fp = fopen("mymap.txt", "r");
		//gold=getline(fp,s);
		//cout << gold << endl;
		if (fp == NULL)
		{
			printf("We can't open the file.");
			fclose(fp);
			return 1;
		}
		else
		{
			while((i=fgetc(fp))!='\n');
			while((i=fgetc(fp))!='\n')
			{
				++columns_in_map;
			}
			while((i=fgetc(fp))!=EOF)
			{
				if(i == '\n')
				++rows_in_map;
			}
			--columns_in_map;
      --rows_in_map;
			//cout<<"ROWS:"<< rows_in_map <<endl;
			//cout<<"COLUMNS:"<< columns_in_map <<endl;
		}
		//use ftruncate to grow the shared memory to match rows*cols
		int trunc = ftruncate(fd, off_t(rows_in_map*columns_in_map));
		//cout << trunc << endl;

		//rows*cols+sizeof(GameBoard)
		goldmap= (GameBoard*)mmap(NULL, (rows_in_map*columns_in_map)  + sizeof(GameBoard), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		goldmap->rows=rows_in_map;
	    goldmap->columns=columns_in_map;
    for(int i=0;i<5;i++)
      goldmap->pid[i]=0;

		int gold;
		char ptr;
		fstream file("mymap.txt");
		file >> gold;
		file.ignore();
		int x= gold; //x recieves the first line in int from the textfile!
		//cout<< x << endl;
		unsigned char* mp=goldmap -> map;
		//Convert the ASCII bytes into bit fields drawn from goldchase.h
		while(file.get(ptr))
		{

			switch(ptr)
			{
				case ' ':	*mp=0;mp++;
							break;
				case '*':	*mp=G_WALL;mp++;
							break;
			}

		}

	    //drop the gold randomly
	   	while (x > 0)
		{
			//cout<<"in while loop" << endl;
			random=(rand() % (rows_in_map*columns_in_map)) ;
			//cout << random << endl;
	        if(goldmap->map[random]==0)
           	{
			 	if(x==1) goldmap->map[random]=G_GOLD;
				else goldmap->map[random]=G_FOOL;
				x--;
	        }
		}
 		sem_post(mysemaphore);
	}
	else
	{
		//sem_unlink("/AAgoldchase");
		//shm_unlink("/shm_obj1");
		mysemaphore=sem_open("/AAgoldchase", O_RDWR, S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR, 1);
		//sem_wait(mysemaphore);
		//sem_post(mysemaphore);

          fd = shm_open("/shm_obj1",O_RDWR , S_IRUSR | S_IWUSR);
	  sem_wait(mysemaphore);
          READ(fd,&rows_in_map,sizeof(int));
          READ(fd,&columns_in_map,sizeof(int));
          goldmap = (GameBoard*)mmap(NULL,(rows_in_map*columns_in_map)  + sizeof(GameBoard),PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
		sem_post(mysemaphore); //cout<<rows_in_map<<columns_in_map<<endl;

    // kill(goldmap->daemonID,SIGHUP);   added
	}

	//Trying to add player 1


		while(j>1)
		{
			//sem_wait(mysemaphore);
			random=(rand() % (rows_in_map*columns_in_map)) ;

			if( goldmap->map[random]== 0)
			{
				if(goldmap->pid[0] == 0)
				{
          goldmap->map[random]|=G_PLR0;
					goldmap->players |= G_PLR0;
					curr_player=G_PLR0;
					curr_pos=random;
          goldmap->pid[0]=getpid();
          mq_name="/myplra1";
          for(int i=0;i<5;i++){
          if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
            kill(goldmap->pid[i],SIGUSR1);}
				}

				else if(goldmap->pid[1] == 0)
				{
					goldmap->map[random]|=G_PLR1;
					goldmap->players |= G_PLR1;
            	    			curr_player=G_PLR1;
					curr_pos=random;
          goldmap->pid[1]=getpid();
          mq_name="/myplra2";
          for(int i=0;i<5;i++){
          if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
            kill(goldmap->pid[i],SIGUSR1);}
				}
				else if(goldmap->pid[2] == 0)
				{
					goldmap->map[random]|=G_PLR2;
					goldmap->players |= G_PLR2;
            	    			curr_player=G_PLR2;
					curr_pos=random;
          goldmap->pid[2]=getpid();
          mq_name="/myplra3";
          for(int i=0;i<5;i++){
          if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
            kill(goldmap->pid[i],SIGUSR1);}
				}
				else if(goldmap->pid[3] == 0)
				{
					goldmap->map[random]|=G_PLR3;
					goldmap->players |= G_PLR3;
            	    			curr_player=G_PLR3;
					curr_pos=random;
          goldmap->pid[3]=getpid();
          mq_name="/myplra4";
          for(int i=0;i<5;i++){
          if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
            kill(goldmap->pid[i],SIGUSR1);}
				}
				else if(goldmap->pid[4] == 0)
				{
					goldmap->map[random]|=G_PLR4;
					goldmap->players |= G_PLR4;
          curr_player=G_PLR2;
					curr_pos=random;
          goldmap->pid[4]=getpid();
          mq_name="/myplra5";
          for(int i=0;i<5;i++){
          if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
            kill(goldmap->pid[i],SIGUSR1);}
				}
				else
				{
					cout<<"Board is full !! Try later !!"<<endl;
					exit(0);
				}

			--j;
			}
				//sem_post(mysemaphore);
		}
       		goldmap->players!=curr_player;
		Map goldMine(goldmap->map,rows_in_map,columns_in_map);
    mapp=&goldMine;
		int a=0;

    if((readqueue_fd=mq_open(mq_name.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
            S_IRUSR|S_IWUSR, &mq_attributes))==-1)
    {
      perror("mq_open");
      exit(1);
    }

    struct sigevent mq_notification_event;
    mq_notification_event.sigev_notify=SIGEV_SIGNAL;
    mq_notification_event.sigev_signo=SIGUSR2;
    mq_notify(readqueue_fd, &mq_notification_event);


		if(goldmap -> daemonID == 0)
		{
			serverStartup();
		}
    else
    {
      kill(goldmap -> daemonID, SIGHUP);
    }

	  	goldMine.postNotice("This is a notice");
			bool goldCheck = false;
  		while((a=goldMine.getKey())!='Q' && !forcequit)
		  {
			sem_wait(mysemaphore);
			if((goldCheck == true) && ((((curr_pos%columns_in_map))== columns_in_map - 1) || ((curr_pos%columns_in_map) == 0)
				|| (curr_pos/columns_in_map == 0) || (curr_pos/columns_in_map == rows_in_map - 1)))
			{
        goldMine.postNotice("YOU WON!!!!");
        sem_post(mysemaphore);
				break;
			}

			if(a=='h')
			{
				if((curr_pos%columns_in_map)!=0 && !(goldmap->map[curr_pos-1] & G_WALL))
				{//cout<<curr_pos<<endl;
					//curr_pos--;
					goldmap->map[curr_pos] &= ~curr_player;
					curr_pos--;
					goldmap->map[curr_pos] |= curr_player;
          goldMine.drawMap();
					if(goldmap->map[curr_pos] & G_GOLD)
					{
						goldMine.postNotice("You found gold!!");
						goldmap->map[curr_pos] &= ~G_GOLD;
						goldCheck = true;
						//break;
					}
					else if(goldmap->map[curr_pos] & G_FOOL)
					{
						goldMine.postNotice("You have got fool's gold!!");
					}

          for(int i=0;i<5; i++)
          {
            if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
              kill(goldmap->pid[i],SIGUSR1);
          }
				}
				goldMine.drawMap();

			}

			if(a=='l')
			{
				if((curr_pos%columns_in_map)!=(columns_in_map-1) && !(goldmap->map[curr_pos+1] & G_WALL))
				{//cout<<curr_pos<<endl;
					//curr_pos--;
					goldmap->map[curr_pos] &= ~curr_player;
					curr_pos++;
					goldmap->map[curr_pos] |= curr_player;goldMine.drawMap();
					if(goldmap->map[curr_pos] & G_GOLD)
					{
						goldMine.postNotice("You found gold!!");
						//goldmap->map[curr_pos]=0;
						goldmap->map[curr_pos] &= ~G_GOLD;
						goldCheck = true;
					//	break;
					}
					else if(goldmap->map[curr_pos] & G_FOOL)
					{
						goldMine.postNotice("You have got fool's gold!!");
					}

          for(int i=0;i<5; i++)
          {
            if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
              kill(goldmap->pid[i],SIGUSR1);
          }
				}
				goldMine.drawMap();

			}
			if(a=='j')
			{
				int row=curr_pos/columns_in_map;
				if((row<rows_in_map-1) && !(goldmap->map[curr_pos+columns_in_map] & G_WALL))
				{//cout<<curr_pos<<endl;
					//curr_pos--;
					goldmap->map[curr_pos] &= ~curr_player;
					curr_pos=curr_pos+columns_in_map;
					goldmap->map[curr_pos] |= curr_player;goldMine.drawMap();
					if(goldmap->map[curr_pos] & G_GOLD)
					{
						goldMine.postNotice("You found gold!!");
						//goldmap->map[curr_pos]=0;
						goldmap->map[curr_pos] &= ~G_GOLD;
						goldCheck = true;
						//break;
					}
					else if(goldmap->map[curr_pos] & G_FOOL)
					{
						goldMine.postNotice("You have got fool's gold!!");
					}

          for(int i=0;i<5; i++)
          {
            if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
              kill(goldmap->pid[i],SIGUSR1);
          }
				}
				goldMine.drawMap();

			}
			if(a=='k')
			{
				int row=curr_pos/columns_in_map;
				if((row>0) && !(goldmap->map[curr_pos-columns_in_map] & G_WALL))
				{//cout<<curr_pos<<endl;
					//curr_pos--;
					goldmap->map[curr_pos] &= ~curr_player;
					curr_pos=curr_pos-columns_in_map;
					goldmap->map[curr_pos] |= curr_player;goldMine.drawMap();
					if(goldmap->map[curr_pos] & G_GOLD)
					{
						goldMine.postNotice("You found gold!!");
						//goldmap->map[curr_pos]=0;
						goldmap->map[curr_pos] &= ~G_GOLD;
						goldCheck = true;
						//break;
					}
					else if(goldmap->map[curr_pos] & G_FOOL)
					{
						goldMine.postNotice("You have got fool's gold!!");
					}

          for(int i=0;i<5; i++)
          {
            if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
              kill(goldmap->pid[i],SIGUSR1);
          }
				}

      }
      if(a == 'b')
      {
        string message = goldMine.getMessage();
        int sender;
        for(int i = 0; i < 5; i++)
        {
          if(goldmap->pid[i] == getpid())
            sender=i+1;
        }
        for(int i=0;i<5;i++)
        {
          if(i != (sender-1) && goldmap->pid[i] != 0)
            write_message(sender,i,message);
        }
      }
      if(a == 'm')
      {
        int to;
        string message;
        unsigned int all_players = goldmap->players;
        all_players &= ~curr_player;
        unsigned int player=goldMine.getPlayer(all_players);

        if(player == G_PLR0)
          to=0;
          if(player == G_PLR1)
            to=1;
            if(player == G_PLR2)
              to=2;
              if(player == G_PLR3)
                to=3;
                if(player == G_PLR4)
                  to=4;

        message = goldMine.getMessage();
        int sender;
        if(curr_player == G_PLR0)
          sender=1;
        else if(curr_player == G_PLR1)
          sender=2;
        else if(curr_player == G_PLR2)
          sender=3;
        else if(curr_player == G_PLR3)
          sender=4;
        else if(curr_player == G_PLR4)
          sender=5;
        write_message(sender,to,message);
      }
				goldMine.drawMap();
			sem_post(mysemaphore);
	}
    sem_post(mysemaphore);
		sem_wait(mysemaphore);
		goldmap->map[curr_pos] &= ~curr_player;
		goldmap->players &= ~curr_player;
		sem_post(mysemaphore);


    if(goldCheck)
    {
      int sender;
      if(curr_player == G_PLR0)
        sender=1;
      else if(curr_player == G_PLR1)
        sender=2;
      else if(curr_player == G_PLR2)
        sender=3;
      else if(curr_player == G_PLR3)
        sender=4;
      else if(curr_player == G_PLR4)
        sender=5;
      string msg;
      msg= " won !";
      for(int i=0;i<5;i++)
      {
        if(goldmap->pid[i] != getpid() && goldmap->pid[i]!=0)
        {
            write_message(sender,i,msg);
        }
      }
    }

              for(int i=0;i<5; i++)
              {
                if((goldmap->pid[i] != 0) && (goldmap->pid[i] != getpid()))
                  kill(goldmap->pid[i],SIGUSR1);

              }
    for(int k = 0; k < 5; k++)
    {
      if(goldmap -> pid[k] == getpid())
        goldmap -> pid[k] = 0;
    }

    mq_close(readqueue_fd);
    mq_unlink(mq_name.c_str());
    kill(goldmap -> daemonID, SIGHUP);
    return 0;
}

void handle_interrupts(int)
{
  unsigned char sendZero = 0;
  short size;

  vector< pair<short,unsigned char> > pvec;
  pair<short,unsigned char> myPair;

  for(short i = 0;i < (goldmapServer->rows * goldmapServer->columns);i++)
  {
    if(goldmapServer->map[i] != local[i])
    {
      myPair.first = i;
      myPair.second = goldmapServer -> map[i];
      pvec.push_back(myPair);
      local[i] = goldmapServer -> map[i];
    }
  }
  if(pvec.size() > 0)
  {
    size = pvec.size();
    WRITE(new_sockfd,&sendZero,sizeof(sendZero));
    WRITE(new_sockfd, &size, sizeof(size));
    for(short i = 0; i < size; i++)
    {
      WRITE(new_sockfd, &(pvec[i].first), sizeof(pvec[i].first));
      WRITE(new_sockfd, &(pvec[i].second), sizeof(pvec[i].second));
    }
  }
}
void clean_ups(int)
{
  unsigned char temp = G_SOCKPLR;
  for(int i = 0; i < 5;i++)
  {
    if((goldmapServer -> pid[i] != 0))
    {
      if(i == 0)
        temp |= G_PLR0;
      else if(i == 1)
        temp |= G_PLR1;
      else if(i == 2)
        temp |= G_PLR2;
      else if(i == 3)
        temp |= G_PLR3;
      else if(i  == 4)
        temp |= G_PLR4;
    }
  }
  WRITE(new_sockfd,&temp,sizeof(temp));
  if(temp == G_SOCKPLR)
  {
    sem_unlink("/AAgoldchase");
    shm_unlink("/shm_obj1");
    exit(0);
  }
}
void read_messages(int)
{


}

void serverStartup()
{
	/* Daemon Creation */
  if(fork() > 0)
	{
		return;
	}

	if(fork() > 0)
		exit(0);

	if(setsid() == -1)
		exit(1);

	for(int i = 0; i < sysconf(_SC_OPEN_MAX); ++i)
		close(i);

	open("/dev/null", O_RDWR);	//fd 0
	open("/dev/null", O_RDWR);	//fd 1
	open("/dev/null", O_RDWR);	//fd 2

	umask(0);
	chdir("/");

   struct sigaction action_jackson;
   action_jackson.sa_handler=handle_interrupts;
   sigemptyset(&action_jackson.sa_mask);
   action_jackson.sa_flags=0;
   action_jackson.sa_restorer=NULL;

   sigaction(SIGUSR1, &action_jackson, NULL);


   struct sigaction exit_handler;
   exit_handler.sa_handler=clean_ups;
   sigemptyset(&exit_handler.sa_mask);
   exit_handler.sa_flags=0;
   sigaction(SIGHUP,&exit_handler,NULL);

   struct sigaction action_to_take;
   action_to_take.sa_handler=read_messages;
   sigemptyset(&action_to_take.sa_mask);
   action_to_take.sa_flags=0;
   sigaction(SIGUSR2, &action_to_take, NULL);

   struct mq_attr mq_attributes;
   mq_attributes.mq_flags=0;
   mq_attributes.mq_maxmsg=10;
   mq_attributes.mq_msgsize=120;

   mysemaphore = sem_open("/AAgoldchase", O_RDWR | O_EXCL, S_IRUSR | S_IWUSR, 1);
   int file_d = shm_open("/shm_obj1",  O_RDWR, S_IRUSR | S_IWUSR);

  int r,c;
  READ(file_d,&r,sizeof(int));
  READ(file_d,&c,sizeof(int));

  goldmapServer = (GameBoard*)mmap(0,(r * c) + sizeof(GameBoard), PROT_READ | PROT_WRITE, MAP_SHARED, file_d,0);
  goldmapServer -> daemonID = getpid();

  local = new unsigned char[(r*c)];

  for(int i = 0; i < (r*c); ++i)
    local[i] = goldmapServer -> map[i];

  //change this # between 2000-65k before using
  const char* portno="52424";
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints)); //zero out everything in structure
  hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
  hints.ai_socktype=SOCK_STREAM; // TCP stream sockets
  hints.ai_flags=AI_PASSIVE; //file in the IP of the server for me

  struct addrinfo *servinfo;
  if((status=getaddrinfo(NULL, portno, &hints, &servinfo))==-1)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  /*avoid "Address already in use" error*/
  int yes=1;
  if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1)
  {
    perror("setsockopt");
    exit(1);
  }

  //We need to "bind" the socket to the port number so that the kernel
  //can match an incoming packet on a port to the proper process
  if((status=bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
  {
    perror("bind");
    exit(1);
  }
  //when done, release dynamically allocated memory
  freeaddrinfo(servinfo);
  if(listen(sockfd,1)==-1)
  {
    perror("listen");
    exit(1);
  }

  struct sockaddr_in client_addr;
  socklen_t clientSize=sizeof(client_addr);
  //int new_sockfd;
  do
    new_sockfd=accept(sockfd, (struct sockaddr*) &client_addr, &clientSize);
  while(new_sockfd == -1 && errno == EINTR);

    if(new_sockfd == -1)
      exit(1);

    WRITE(new_sockfd,&r,sizeof(int));
    WRITE(new_sockfd,&c,sizeof(int));
    for(int i = 0; i < (r * c); ++i)
    {
      WRITE(new_sockfd,&local[i],sizeof(local[i]));
    }

    unsigned char play = G_SOCKPLR;
    for(int i = 0; i < 5;i++)
    {
      if((goldmapServer->pid[i] != 0))
      {
        if(i == 0)
          play |= G_PLR0;
        if(i == 1)
          play |= G_PLR1;
        if(i == 2)
          play |= G_PLR2;
        if(i == 3)
          play |= G_PLR3;
        if(i  == 4)
          play |= G_PLR4;
      }
    }

    WRITE(new_sockfd,&play,sizeof(play));
    unsigned char ar[5] = {G_PLR0,G_PLR1,G_PLR2,G_PLR3,G_PLR4};
    string arnames[5] = {"/myplra1","/myplra2","/myplra3","/myplra4","/myplra5"};
    while(1)
    {
      unsigned char myByte;
      READ(new_sockfd,&myByte,sizeof(myByte));

      if(myByte & G_SOCKPLR)
      {
        for(int i = 0; i < 5; i++)
        {
        if((myByte & ar[i]) && (goldmapServer ->pid[i] == 0))
        {
          if((queueArray[i] = mq_open(arnames[i].c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
                  S_IRUSR|S_IWUSR, &mq_attributes))==-1)
          {
            exit(1);
          }

          struct sigevent mq_notification_event;
          mq_notification_event.sigev_notify=SIGEV_SIGNAL;
          mq_notification_event.sigev_signo=SIGUSR2;
          mq_notify(queueArray[i], &mq_notification_event);
          goldmapServer->pid[i] = getpid();
        }
        else if(!(myByte & ar[i]) && (goldmapServer->pid[i] != 0))
        {
          mq_close(queueArray[i]);
          mq_unlink(arnames[i].c_str());
          goldmapServer->pid[i] = 0;
        }

        }
        if(myByte == G_SOCKPLR)
        {
          sem_unlink("/AAgoldchase");
          shm_unlink("/shm_obj1");
          exit(1);
        }


      }
      else if(myByte & G_SOCKMSG)
      {





      }
      else if(myByte == 0)
      {
        short number;
        READ(new_sockfd, &number, sizeof(number));
        for(short i = 0;i < number ;i++)
        {
          short index;
          unsigned char diff;
          READ(new_sockfd, &index, sizeof(index));
          READ(new_sockfd, &diff, sizeof(diff));
          local[index] = diff;
          goldmapServer -> map[index] = diff;
        }
        for(int i = 0;i < 5;i++)
        {
          if((goldmapServer->pid[i] !=0) && (goldmapServer->pid[i] !=getpid()))
            kill(goldmapServer -> pid[i],SIGUSR1);
        }
      }
    }
  }



void handle_interruptc(int)
{
  vector< pair<short,unsigned char> > pvec;
  pair<short,unsigned char> myPair;

  for(int i=0;i < (goldmapClient -> rows * goldmapClient -> columns); i++)
  {
    if(goldmapClient -> map[i] != local[i])
    {
      myPair.first = i;
      myPair.second = goldmapClient -> map[i];
      pvec.push_back(myPair);
      local[i] = goldmapClient -> map[i];
    }
  }
  if(pvec.size() > 0)
  {
    unsigned char temp = 0;
    short size = (short)pvec.size();
    WRITE(sockfd,&temp,sizeof(temp));
    WRITE(sockfd, &size, sizeof(size));
    for(short i = 0; i < size; i++)
    {
      WRITE(sockfd, &(pvec[i].first), sizeof(pvec[i].first));
      WRITE(sockfd, &(pvec[i].second), sizeof(pvec[i].second));
    }
  }


}
void clean_upc(int)
{
  unsigned char temp = G_SOCKPLR;
  for(int i = 0; i < 5;i++)
  {
    if((goldmapClient -> pid[i] != 0))
    {
      if(i == 0)
        temp |= G_PLR0;
      else if(i == 1)
        temp |= G_PLR1;
      else if(i == 2)
        temp |= G_PLR2;
      else if(i == 3)
        temp |= G_PLR3;
      else if(i  == 4)
        temp |= G_PLR4;
    }
  }
  WRITE(sockfd,&temp,sizeof(temp));
  if(temp == G_SOCKPLR)
  {
    sem_unlink("/AAgoldchase");
    shm_unlink("/shm_obj1");
  }
}
void read_messagec(int)
{

}

void clientStartup(string a)
{
  pipe(pipefd);
 if(fork() > 0)
 {
   close(pipefd[1]); //close write, parent only needs read
   int val;
   READ(pipefd[0], &val, sizeof(val));
   if(val==1)
     WRITE(1, "Success!\n", sizeof("Success!\n"));
   else
     WRITE(1, "Failure!\n", sizeof("Failure!\n"));
   return;
 }

 if(fork() > 0)
   exit(0);

 if(setsid() == -1)
   exit(1);

 for(int i = 0; i < sysconf(_SC_OPEN_MAX); ++i)
{
    if(i!=pipefd[1])//close everything, except write
      close(i);
}

 open("/dev/null", O_RDWR);	//fd 0
 open("/dev/null", O_RDWR);	//fd 1
 open("/dev/null", O_RDWR);	//fd 2

 umask(0);
 chdir("/");

 struct sigaction action_jackson;
 action_jackson.sa_handler=handle_interruptc;
 sigemptyset(&action_jackson.sa_mask);
 action_jackson.sa_flags=0;
 action_jackson.sa_restorer=NULL;

 sigaction(SIGUSR1, &action_jackson, NULL);


 struct sigaction exit_handler;
 exit_handler.sa_handler=clean_upc;
 sigemptyset(&exit_handler.sa_mask);
 exit_handler.sa_flags=0;
 sigaction(SIGHUP,&exit_handler,NULL);

 struct sigaction action_to_take;
 action_to_take.sa_handler=read_messagec;
 sigemptyset(&action_to_take.sa_mask);
 action_to_take.sa_flags=0;
 sigaction(SIGUSR2, &action_to_take, NULL);


 struct mq_attr mq_attributes;
 mq_attributes.mq_flags=0;
 mq_attributes.mq_maxmsg=10;
 mq_attributes.mq_msgsize=120;

   const char* portno="52424";
//
  struct addrinfo hints;
   memset(&hints, 0, sizeof(hints)); //zero out everything in structure
   hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
   hints.ai_socktype=SOCK_STREAM;
 struct addrinfo *servinfo;

 //instead of "localhost", it could by any domain name
 if((status=getaddrinfo(a.c_str(), portno, &hints, &servinfo))==-1)
 {
   fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
   exit(1);
 }
 sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

 if((status=connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
 {
   perror("connect");
   exit(1);
 }
 //release the information allocated by getaddrinfo()
 freeaddrinfo(servinfo);

 int r,c;
 READ(sockfd,&r,sizeof(int));
 READ(sockfd,&c,sizeof(int));

//READ nbytes that is rows*cols in to the local space.
 local = new unsigned char[(r*c)];
 for(int i = 0; i < (r*c); ++i)
 {
   READ(sockfd,&local[i],sizeof(local[i]));
 }
mysemaphore= sem_open("/AAgoldchase", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);

//create and initialize shared memory from your local copy of the map ( new shared memory opened)
int fd = shm_open("/shm_obj1", O_CREAT|O_RDWR,S_IRUSR | S_IWUSR);
int trunc = ftruncate(fd, off_t(r*c)  + sizeof(GameBoard));


goldmapClient = (GameBoard*)mmap(0,(r * c) + sizeof(GameBoard), PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
goldmapClient -> daemonID = getpid();
goldmapClient -> rows = r;
goldmapClient -> columns = c;

 for(int i = 0; i < (r*c); ++i)
 {
  unsigned char temp = local[i];
   goldmapClient -> map[i] = temp;
 }
 unsigned char ar[5] = {G_PLR0,G_PLR1,G_PLR2,G_PLR3,G_PLR4};
 string arnames[5] = {"/myplra1","/myplra2","/myplra3","/myplra4","/myplra5"};
bool check = false;

while(1)
{
  unsigned char myByte;
  READ(sockfd,&myByte,sizeof(myByte));

  if(myByte & G_SOCKPLR)
  {
    for(int i = 0; i < 5; i++)
    {
    if((myByte & ar[i]) && (goldmapClient ->pid[i] == 0))
    {
      if((queueArray[i] = mq_open(arnames[i].c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
              S_IRUSR|S_IWUSR, &mq_attributes))==-1)
      {
        exit(1);
      }

      struct sigevent mq_notification_event;
      mq_notification_event.sigev_notify=SIGEV_SIGNAL;
      mq_notification_event.sigev_signo=SIGUSR2;
      mq_notify(queueArray[i], &mq_notification_event);
      goldmapClient->pid[i] = getpid();
    }
    else if(!(myByte & ar[i]) && (goldmapClient ->pid[i] != 0))
    {
      mq_close(queueArray[i]);
      mq_unlink(arnames[i].c_str());
      goldmapClient->pid[i] = 0;
    }

    }
    if(myByte == G_SOCKPLR)
    {
      sem_unlink("/AAgoldchase");
      shm_unlink("/shm_obj1");
      exit(1);
    }
    if(check == false)
    {
      int val = 1;
      WRITE(pipefd[1], &val, sizeof(val));
      check = true;
    }
  }
  else if(myByte & G_SOCKMSG)
  {





  }
  else if(myByte == 0)
  {
    short number;
    READ(sockfd, &number, sizeof(number));
    for(short i = 0;i < number ;i++)
    {
      short index;
      unsigned char diff;
      READ(sockfd, &index, sizeof(index));
      READ(sockfd, &diff, sizeof(diff));
      local[index] = diff;
      goldmapClient -> map[index] = diff;
    }
    for(int i = 0;i < 5;i++)
    {
      if(goldmapClient->pid[i] !=0 && goldmapClient->pid[i] !=getpid())
        kill(goldmapClient -> pid[i],SIGUSR1);
    }
    }
  }
}
