#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include "restart.h"
#include "uici.h"

#define THREAD_TABLE_SIZE 512

typedef struct
{
	int communfd;
	pid_t clientpid;
	int clientid;
	int clientflag;
}threadPacket;

static int signalFlag;
static sem_t semaphore;

threadPacket threadTable[THREAD_TABLE_SIZE];
int threadNbr;
int listenfd;

static void signalHandler(int sigId);
void* threadFunction(void* arg);
void printWithTime(char* text);
void clientFunction(int communfd, pid_t clientpid);


int main(int argc, char *argv[])
{
	char client[MAX_CANON];
	int communfd, i;
	u_port_t portnumber;
	pthread_t threadIds[THREAD_TABLE_SIZE];
	
	if (argc != 3) 
	{
		fprintf(stderr, "Usage: %s <port #, id> <thpool size, k >\n", argv[0] );
		return 1;
	}
	
	portnumber = (u_port_t) atoi(argv[1]);
	
	if ((listenfd = u_open(portnumber)) == -1) 
	{
		perror("Failed to create listening endpoint");
		return 1;
	}
	
	fprintf(stderr, "[%ld]: Waiting for the first connection on port %d\n",
			(long)getpid(), (int)portnumber);
	
	for (i=0; i<THREAD_TABLE_SIZE; i++)
	{
		threadTable[i].clientflag = 0;
	}
	
	sem_init(&semaphore, 0, 1);
	
	signalFlag = 1;
	signal(SIGINT, signalHandler);
	
	threadNbr = 0;
	while (signalFlag)
	{
		if ((communfd = u_accept(listenfd, client, MAX_CANON)) == -1 && signalFlag)
		{
			perror("Failed to accept connection");
			continue;
		}
		
		if (signalFlag)
		{
			threadTable[threadNbr].clientid = threadNbr;
			threadTable[threadNbr].communfd = communfd;
			if (0 != pthread_create(&threadIds[threadNbr], NULL, &threadFunction, &threadTable[threadNbr]))
			{
				printf("There is an error while creating thread.\n");
			}
			
			threadNbr++;
		}
	}
	
	for (i=0; i<threadNbr; i++)
	{
		pthread_join(threadIds[i], NULL);
	}
	
	sem_destroy(&semaphore);
	
	return 0;
}

void signalHandler(int sigId)
{
	if (sigId == SIGINT)
	{
		printf("Catched SIGINT(%d) signal.\n",sigId);
		signalFlag = 0;
		
		if (r_close(listenfd) == -1)
		{
			printf("failed to close communfd: %s\n", strerror(errno));
		}
	}
}

void* threadFunction(void* arg)
{
	threadPacket* packetp = (threadPacket*) arg;
	threadPacket packet;
	char readBuf[4096];
	int readsize;
	

	packet.clientid = packetp->clientid;
	packet.communfd = packetp->communfd;
	readsize = r_read(packet.communfd, readBuf, 4096);
	packet.clientpid = atoi(readBuf);
	
	packetp->clientpid = packet.clientpid;
	packetp->clientflag = 1;
	
	sprintf(readBuf, "%d", (int)pthread_self());
	message_write(packet.communfd, readBuf, strlen(readBuf), packet.clientpid);
	
	printf("Client%d [%d] Thread [%d] ye", packet.clientid, packet.clientpid, (int)pthread_self());
	printWithTime(" baglandi.");
	
	while (signalFlag && readsize != -2)
	{
		memset(readBuf, 0, sizeof(readBuf));
		readsize = message_readtimed(packet.communfd, readBuf, 4096, 0.1, pthread_self());
		if (readsize > 0)
		{
			 if (0 == strcmp(readBuf, ""))
			{
				printf("Client[%d]", packet.clientpid);
				printWithTime("");
				clientFunction(packet.communfd, packet.clientpid);
			}
			
			else
			{
				r_write(STDOUT_FILENO, readBuf, readsize);
			}
			

		}
	}
	
	printf("Thread[%d] client[%d] koptu.\n", (int)pthread_self(), packet.clientpid);
	
	if (r_close(packet.communfd) == -1)
	{
		printf("[%d] : failed to close communfd: %s\n", (int)pthread_self(), strerror(errno));
	}
	
	packetp->clientflag = 0;
	
	kill (packet.clientpid, SIGINT);

	return NULL;
}

void printWithTime(char* text)
{
	struct tm * tm;
	struct timeval tval;
	
	gettimeofday(&tval, NULL);
	tm = localtime ( &tval.tv_sec );
	
	printf("%s at [%d] %s", text, (int)tval.tv_usec, asctime (tm));
}


void clientFunction(int communfd, pid_t clientpid)
{
	char client[1024];
	int i;

	message_write(communfd, &threadNbr, sizeof(int), clientpid);

	sem_wait(&semaphore);
	
	for (i=0; i<threadNbr; i++)
	{
		if (threadTable[i].clientflag == 1)
		{
			sprintf(client, "[pid:%d][id:%d]", threadTable[i].clientpid, threadTable[i].clientid);
			message_write(communfd, client, strlen(client), clientpid);
		}
	}
	sem_post(&semaphore);
}
