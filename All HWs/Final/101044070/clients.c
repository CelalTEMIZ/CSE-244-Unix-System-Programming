#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "restart.h"
#include "uici.h"


static int signalFlag;

static void signalHandler(int sigId);
void printWithTime(char* text);
void clientFunction(int communfd, pthread_t connectedThreadId);


int main(int argc, char *argv[])
{
	int communfd;
	u_port_t portnumber;
	char readBuf[4096];
	int readsize;
	pthread_t connectedThreadId;
	
	if (argc != 6)
	{
		fprintf(stderr, "Usage: %s <127.0.0.1> <port #, id> <#of columns of A, m> <#of rows of A, p> <#of clients, q> \n", argv[0]);
		return 1;
	}
	
	portnumber = (u_port_t)atoi(argv[2]);
	
	if ((communfd = u_connect(portnumber, argv[1])) == -1)
	{
		printf("Waiting Server...\n");
		while((communfd = u_connect(portnumber, argv[1])) == -1)
		{
		}
	}
	
	sprintf(readBuf, "%d", (int)getpid());
	r_write(communfd, readBuf, strlen(readBuf));
	
	memset(readBuf, 0, sizeof(readBuf));
	message_read(communfd, readBuf, (int)getpid());
	connectedThreadId = atoi(readBuf);
	
	printf("Client[%d] %s(%d)'e baglandi.\n", (int)getpid(), argv[1], (int)connectedThreadId);
	
	signalFlag = 1;
	signal(SIGINT, signalHandler);
	
	while (signalFlag != 0 && readsize != -2)
	{
		memset(readBuf, 0, sizeof(readBuf));
		readsize = readtimed(STDIN_FILENO, readBuf, 4096, 0.1);
		if (readsize > 0)
		{
			
			{
				message_write(communfd, readBuf, readsize, connectedThreadId);
			}
		}
	}
	
	return 0;
}

static void signalHandler(int sigId)
{
	if (sigId == SIGINT)
	{
		printf("Catched SIGINT(%d) signal.\n", sigId);
		signalFlag = 0;
	}
}

void printWithTime(char* text)
{
	struct tm * tm;
	struct timeval tval;
	
	gettimeofday(&tval, NULL);
	tm = localtime ( &tval.tv_sec );
	
	printf("%s at [%d] %s", text, (int)tval.tv_usec, asctime (tm));
}


void clientFunction(int communfd, pthread_t connectedThreadId)
{
	char client[1024];
	int clientNbr, i;
	
	sprintf(client, "--");
	message_write(communfd, client, strlen(client), connectedThreadId);
	
	message_read(communfd, &clientNbr, getpid());

	for (i=0; i<clientNbr; i++)
	{
		memset(client, 0, sizeof(client));
		message_read(communfd, client, getpid());
		printf("Client : %s\n", client);
	}
	
}


