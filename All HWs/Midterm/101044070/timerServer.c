#include <time.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAX_BUF 4096
#define timerServerLogFILE "timerServerLog.txt"
FILE *TIMERSERVERLOGFILE;


int numberOfClient=0;
long int timerServerPID;
char BUF[MAX_BUF];
char tempPidS[10];
struct timeval timeSpecBegin, timeSpecEnd;
int timerServerFlag=0;

double initialMatrix[80][80];	
double copyInitialMatrix[80][80];

char* MAINFIFO;
char CONNECTED_CLIENT_FIFO[1024]; 
int fileDescConnectedClient;
int fileDescMainFifo;

void   usage();
double computeDeterminant(double inputMatrix[80][80], int inputMatrixSize);
void   sigHandler(int signo);
void   runTimerServer(int argNumber, char **argv);
void   alarmHandler(int sig);


int main(int argc, char *argv[])
{

		runTimerServer(argc, argv);
	
		return 0;
}				

void usage()
{
	fprintf(stderr, "Usage: ./timerServer  < ticts msec >  < n >  < mainFifoName >\n");
}



void  runTimerServer(int argNumber, char **argv)
{


	pid_t pid;
	timerServerPID=(long)getpid();
	int fCounter=0;
	time_t currentTime;
	time_t lastTime;
	struct tm *timerServerTime;
	currentTime = time(NULL);
	
	long int miliSecondsClient;
	long int totalTime=0;
	double resultOfDeterminanCalculation=0;


	long int mSec = atol(argv[1]);
	int firstMatrixSize = atoi(argv[2]);
	int newMatrixSize=2 * firstMatrixSize;

	int i,j,k,l;

	long int value=0;
		

	signal(SIGINT, &sigHandler);
	
		
	if (argNumber != 4)	
	{
		usage();
		exit(0);
	}


	if(mSec == 0)
	{
		printf("Invalid Time Value : \n");
		exit(0);
	}
	
	timerServerTime = localtime( &currentTime );
	fprintf(stderr,"Server Pid : %ld \nStarting Time is %.2d:%.2d:%.2d\nWaiting for the clients .. \n",
	(long)getpid(), timerServerTime->tm_hour, timerServerTime->tm_min, timerServerTime->tm_sec );


	MAINFIFO=argv[3];
	mkfifo(MAINFIFO, 0666);//kendi pid sini fifo yapar timerServerPID
	fileDescMainFifo = open(MAINFIFO,O_RDONLY);
	
	 if((TIMERSERVERLOGFILE=fopen(timerServerLogFILE,"a")) != NULL){
		fprintf(TIMERSERVERLOGFILE,"[Server:%ld] Starting Time is %.2d:%.2d:%.2d\n",
		(long)getpid(),timerServerTime->tm_hour, timerServerTime->tm_min, timerServerTime->tm_sec );
		
		}

	while(1)
	{
			
			signal(SIGALRM, alarmHandler);
			signal(SIGINT, &sigHandler); 
			
			if(read(fileDescMainFifo,BUF,MAX_BUF) != 0)
			{
				numberOfClient++;
					
				if ((pid = fork()) == -1)
				{
					perror("Fork Errror \n");
					exit(0);
				}
				
				else if(pid == 0)
				{
				
					alarm(1000);
					signal(SIGINT, &sigHandler); 
				 	totalTime=0;
				 	miliSecondsClient=mSec;//milisaniye
				 	strncpy(tempPidS,BUF+1,10);//get pid from buf
					printf("Client [%ld] Connected:::%d\n",(long)getpid(), numberOfClient); 	
					
					gettimeofday (&timeSpecBegin, NULL);
					snprintf(CONNECTED_CLIENT_FIFO,20,"cli%ld",atol(tempPidS));//cliente özel fifo	
					mkfifo(CONNECTED_CLIENT_FIFO, 0666);//int fifo
					fileDescConnectedClient = open(CONNECTED_CLIENT_FIFO,O_WRONLY);	
					if (fileDescConnectedClient < 0)
					{
						perror("Fifo couldn't opened:\n");
						exit(0);
					}
				    time( &currentTime );
					timerServerTime = localtime( &currentTime ); 	
					
					snprintf(BUF,MAX_BUF,"%ld \n",(long)getppid());
					write(fileDescConnectedClient,BUF,MAX_BUF);
					
					for ( ; ; )
					{
							
							usleep(mSec*2000);	
							totalTime += mSec;
							fCounter++;
							gettimeofday (&timeSpecEnd, NULL);
							value=((timeSpecEnd.tv_sec - timeSpecBegin.tv_sec)*1000 +
									(timeSpecEnd.tv_usec - timeSpecBegin.tv_usec )/1000 ); 
									
							fprintf(TIMERSERVERLOGFILE," Time = %ld,", value);		

							if ( value < miliSecondsClient)
							{
								time( &lastTime );
								timerServerTime = localtime( &lastTime );
								
								exit(1);
							}
														

							srand(time(NULL));
							    
							    fprintf(TIMERSERVERLOGFILE," [ ");	
							    
								for( i=0;i<newMatrixSize;i++)
								{
									for( j=0;j<newMatrixSize;j++)
									{
										initialMatrix[i][j]= 1+rand()%10;
										fprintf(TIMERSERVERLOGFILE,"%10.2lf",initialMatrix[i][j]);
										fprintf(stderr,"%10.2lf", initialMatrix[i][j]);
									}
								}
								
								for( i=0;i<newMatrixSize;i++)
								{
									for( j=0;j<newMatrixSize;j++)
									{
										 copyInitialMatrix[i+1][j+1]=initialMatrix[i][j];		 
									}
								}
								
								resultOfDeterminanCalculation=computeDeterminant(copyInitialMatrix,newMatrixSize);
								
								fprintf(TIMERSERVERLOGFILE," ],");
								fprintf(TIMERSERVERLOGFILE,"%ld,",(long)getpid());
								fprintf(TIMERSERVERLOGFILE,"%f\n",resultOfDeterminanCalculation);
												
							for(k=0;k<newMatrixSize;k++)
							{
								for(l=0;l<newMatrixSize;l++)
								{								
									snprintf(BUF,MAX_BUF,"%f", initialMatrix[k][l]);	
									write(fileDescConnectedClient,BUF,MAX_BUF);	
									
								}							
							}

							snprintf(BUF,MAX_BUF,"%d", newMatrixSize+1000);	
							write(fileDescConnectedClient,BUF,MAX_BUF);
									
							if(write(fileDescConnectedClient,BUF,MAX_BUF) != MAX_BUF){
								printf("Stopped ! \n");
								exit(0);
							}
					
							
					}
					close(fileDescConnectedClient);
					unlink(CONNECTED_CLIENT_FIFO);
				
			 	}
				else if(pid > 0)
				{
					wait(NULL);
						
				}
		  }
		}
		
		fclose(TIMERSERVERLOGFILE);
}


void sigHandler(int signo)
{
	time_t currentTime;
	struct tm *timerServerTime;
	currentTime = time(NULL);
	timerServerTime = localtime(&currentTime);

	if(timerServerPID == (long)getpid() )
	{
			printf("\nCtrl-C Handled and Child process are killed !\nDie time %.2d:%.2d:%.2d\n",
			timerServerTime->tm_hour, timerServerTime->tm_min, timerServerTime->tm_sec );

			if (numberOfClient > 0)
			{
				close(fileDescConnectedClient);
			}
			unlink(MAINFIFO);
	}
	
	else
	{
			printf("[%ld] Child proces are killed\n", (long)getpid());
	}	

		kill(atol(tempPidS), SIGINT);	

	exit(1);
}

double computeDeterminant(double inputMatrix[80][80], int inputMatrixSize)
{
	double result=0.0;
	double oneDimensionalArray[80];
	double temp,copyDeterminantMatrix[80][80];
	
	int i,j,k,l;

  	if( inputMatrixSize == 2) {

    	return ( inputMatrix[1][1] * inputMatrix[2][2] ) - (inputMatrix[2][1] * inputMatrix[1][2] );
    }

  	else {

    	for(i=1; i<=inputMatrixSize; i++) {   

      		int m=1,n=1;

			for(j=1; j<=inputMatrixSize;j++){

				 for(k=1; k<=inputMatrixSize;k++) {

				    if(j!=1 && k!=i) {

				      copyDeterminantMatrix[m][n] = inputMatrix[j][k];

				      n++;

				     if(n>inputMatrixSize-1) {

				       m++;

				        n=1;
				    }

			}
	}
				         }
     		for(l=1,temp=1; l<=(1+i); l++)
     		
     		temp=(-1)*temp;

     		oneDimensionalArray[i] = temp * computeDeterminant(copyDeterminantMatrix, inputMatrixSize-1);
     }

	     for(i=1,result=0;i<=inputMatrixSize;i++) {

	       result=result+(inputMatrix[1][i]*oneDimensionalArray[i]);
	     }

		
     return result;

	}
}
void  alarmHandler(int sig)
{
  signal(SIGALRM, SIG_IGN);          /* ignore this signal       */
  printf("TimeOut %d \n", getpid());

  signal(SIGALRM, alarmHandler);     /* reinstall the handler    */
  timerServerFlag=1;

}
