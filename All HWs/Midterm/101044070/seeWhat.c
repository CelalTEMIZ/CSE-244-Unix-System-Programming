#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAX_BUF 4096
#define clientLogFile "ClientLogFile.txt"

#define kernelSize 3

char MAINFIFO[MAX_BUF];
char CONNECTED_CLIENT_FIFO[MAX_BUF];
char BUF[MAX_BUF];
char BUF_CLIENT[MAX_BUF];


char clientToResult_FIFO[] = "clientToResult_FIFONAME";
int  clientToResult_ID; 
struct timeval timeSpecBegin_1, timeSpecEnd_1,timeSpecBegin_2, timeSpecEnd_2;

static double part0_Invertible[80][80];
static double part1_Invertible[80][80];
static double part2_Invertible[80][80];
static double part3_Invertible[80][80];

double part0_unInvertible[80][80];
double part1_unInvertible[80][80];
double part2_unInvertible[80][80];
double part3_unInvertible[80][80];


double shiftMatrix[80][80];
double copyShiftMatrix[80][80];
double convolutionMatrix[80][80];
double copyConvolutionMatrix[80][80];


double kernel[kernelSize][kernelSize]={{0,-1,-1},{1,0,-1},{1,1,0}};
 
double bufferedMatrixElements[MAX_BUF];
double copyBufferedMatrixElements[80][80];

int fileDescConnectedClient;
int fileDescMainFifo;
FILE *CLIENTLOGFILE;

static long int timerServerPID;
long int pidChild;
long int processPID;


void usage()
{
	fprintf(stderr,"Usage: ./seeWhat < mainFifoName > \n");
}

void sigHandler(int signo)
{
		kill(pidChild,SIGINT);
		fprintf(stderr,"pidSer %ld",timerServerPID);
		kill(timerServerPID,SIGINT);
		
		close(fileDescConnectedClient);
		unlink(CONNECTED_CLIENT_FIFO);
		printf("Client Ctrl+C handled and Exited\n");
		fclose(CLIENTLOGFILE);
		exit(signo);
}

void computeShiftInverse_nXn(int partID, double inputMatrix[80][80], int matrixNewSize_2nX2n)
{

	int i,j,k,x=0,l=0;
	double ratio,a;
	static double tempInvertible[80][80];
	
    for(i = 0; i < matrixNewSize_2nX2n; i++){
        for(j = 0; j < matrixNewSize_2nX2n; j++){
        		
        		tempInvertible[i][j]= inputMatrix[i][j];
        }
    }
	
    for(i = 0; i < matrixNewSize_2nX2n; i++){
        for(j = matrixNewSize_2nX2n; j < 2*matrixNewSize_2nX2n; j++){
            if(i==(j-matrixNewSize_2nX2n))
                tempInvertible[i][j] = 1.0;
            else
                tempInvertible[i][j] = 0.0;
        }
    }

    for(i = 0; i < matrixNewSize_2nX2n; i++){
        for(j = 0; j < matrixNewSize_2nX2n; j++){
            if(i!=j){
                ratio = tempInvertible[j][i]/tempInvertible[i][i];
                for(k = 0; k < 2*matrixNewSize_2nX2n; k++){
                    tempInvertible[j][k] -= ratio * tempInvertible[i][k];
                   
                }
            }
        }
    }

    for(i = 0; i < matrixNewSize_2nX2n; i++){
        a = tempInvertible[i][i];
        for(j = 0; j < 2*matrixNewSize_2nX2n; j++){
        
            tempInvertible[i][j]=tempInvertible[i][j]/a;
           
        }
    }
    
    for(i = 0; i < matrixNewSize_2nX2n; i++){

        for(j = matrixNewSize_2nX2n; j < 2*matrixNewSize_2nX2n; j++){
            if(partID == 0)
        		part0_Invertible[x][l]=tempInvertible[i][j];
        	else if(partID == 1)
        		part1_Invertible[x][l]=tempInvertible[i][j];
        	else if(partID == 2)
        		part2_Invertible[x][l]=tempInvertible[i][j];  
        	else if(partID == 3)
        		part3_Invertible[x][l]=tempInvertible[i][j]; 
        	   
            l++;
        }
        x++;
        l=0;
    }   
    	
}



double computeDeterminant(double inputMatrix[80][80], int matrixNewSize_2nX2n)
{
	double result=0.0;
	double oneDimensionalArray[80];
	double temp,copyDeterminantMatrix[80][80];
	
	int i,j,k,l;


  	if( matrixNewSize_2nX2n == 2) {

    	return ( inputMatrix[1][1] * inputMatrix[2][2] ) - (inputMatrix[2][1] * inputMatrix[1][2] );
    }

  	else {

    for(i=1; i<=matrixNewSize_2nX2n; i++) {   

     int m=1,n=1;

		for(j=1; j<=matrixNewSize_2nX2n;j++){

			for(k=1; k<=matrixNewSize_2nX2n;k++) {

				 if(j!=1 && k!=i) {

				   copyDeterminantMatrix[m][n] = inputMatrix[j][k];

				    n++;

				  if(n > matrixNewSize_2nX2n-1) {

				  m++;
				  n=1;
				  
				  }

			}
	}
				         }
     		for(l=1,temp=1; l<=(1+i); l++)
     		
     		temp=(-1)*temp;

     		oneDimensionalArray[i] = temp * computeDeterminant(copyDeterminantMatrix, matrixNewSize_2nX2n-1);
     }

	     for(i=1,result=0;i<=matrixNewSize_2nX2n;i++) {

	       result=result+(inputMatrix[1][i]*oneDimensionalArray[i]);
	     }

		
     return result;

	}
}

int circular(int M,int x)
{
    if(x<0)
        return -x-1;
    if(x>=M)
        return 2*M-x-1;

    return x;
}

void computeConvolutionMatrix(int partID, double inputMatrix[80][80], int sizeMatrix)
{

    double tempSum;
    int x1,y1,i;
    int x,y,k,j,a,b;
    double temp[sizeMatrix][sizeMatrix];

    for(y=0;y<sizeMatrix;y++)
    {
        for(x=0;x<sizeMatrix;++x)
        {
            tempSum=0.0;
            for(k=-1; k<=1; k++)
            {
                for(j=-1; j<=1;j++)
                {
                    x1= circular(sizeMatrix, x-j);
                    y1= circular(sizeMatrix, y-k);
                    tempSum= tempSum+ kernel[j+1][k+1]*inputMatrix[y1][x1];

                }

            }
            temp[y][x]=tempSum;
        }

    }
    
    for(i = 0; i < sizeMatrix; i++){
        for(j = 0; j < sizeMatrix; j++){
           
            if(partID==0)
        		part0_Invertible[i][j]=temp[i][j];
        	else if(partID==1)
        		part1_Invertible[i][j]=temp[i][j];
        	else if(partID==2)
        		part2_Invertible[i][j]=temp[i][j];  
        	else if(partID==3)
        		part3_Invertible[i][j]=temp[i][j]; 
        	   
        }
     
    }   
    
    	

}

double divideMatrix_nXn(int operationID, double inputMatrix[80][80], int matrixNewSize_2nX2n )
{
	
	double ratio,a;
	
	double result1=0.0,result2=0.0;
	double copyInputMatrix[80][80];

	int i,j,k=0,l=0;

		for(i=0; i<matrixNewSize_2nX2n/2; ++i)
		{	
			for(j=0; j<matrixNewSize_2nX2n/2; ++j)
			{
				part0_unInvertible[k][l] = inputMatrix[i][j];
				l++;
			}
			k++;
			l=0;
		}
		k=0; 
		l=0;
		
		for(i=0; i<matrixNewSize_2nX2n/2; ++i)
		{
			for(j=matrixNewSize_2nX2n/2; j<matrixNewSize_2nX2n; ++j)
			{		
				part1_unInvertible[k][l] = inputMatrix[i][j];
				l++;
			}
			k++;
			l=0;
		}
		k=0; 
		l=0;
		
		for(i=matrixNewSize_2nX2n/2; i<matrixNewSize_2nX2n; ++i)
		{
			for(j=0; j<matrixNewSize_2nX2n/2; ++j)
			{
				part2_unInvertible[k][l] = inputMatrix[i][j];
				l++;
			}
			k++;
			l=0;
		}
		k=0;
		l=0;
			
		
		for(i=matrixNewSize_2nX2n/2; i<matrixNewSize_2nX2n; ++i)
		{
			for(j=matrixNewSize_2nX2n/2; j<matrixNewSize_2nX2n; ++j)
			{
			 	part3_unInvertible[k][l] = inputMatrix[i][j];	
				l++;	
			}
			k++;
			l=0;
		}
		k=0; 
		l=0;
		
		
		if(operationID == 1)
		{
	   		computeShiftInverse_nXn(0,part0_unInvertible,matrixNewSize_2nX2n/2);
	   		computeShiftInverse_nXn(1,part1_unInvertible,matrixNewSize_2nX2n/2);
	   		computeShiftInverse_nXn(2,part2_unInvertible,matrixNewSize_2nX2n/2);
	   		computeShiftInverse_nXn(3,part3_unInvertible,matrixNewSize_2nX2n/2);	
		}
		
		else if(operationID == 2)
		{
	    	computeConvolutionMatrix(0,part0_unInvertible,matrixNewSize_2nX2n/2);
	    	computeConvolutionMatrix(1,part1_unInvertible,matrixNewSize_2nX2n/2);
	    	computeConvolutionMatrix(2,part2_unInvertible,matrixNewSize_2nX2n/2);
	    	computeConvolutionMatrix(3,part3_unInvertible,matrixNewSize_2nX2n/2);
	    }
		
		k=0; 
		l=0;
	
	
		for(i=0; i<matrixNewSize_2nX2n/2; ++i)
		{	
			for(j=0; j<matrixNewSize_2nX2n/2; ++j)
			{
				if(operationID==1)
				shiftMatrix[i][j] = part0_Invertible[k][l];
				else if(operationID==2)
				convolutionMatrix[i][j]= part0_Invertible[k][l];
				
				l++;
			}
			k++;
			l=0;
		}
		k=0; 
		l=0;
		
		for(i=0; i<matrixNewSize_2nX2n/2; ++i)
		{
			for(j=matrixNewSize_2nX2n/2; j<matrixNewSize_2nX2n; ++j)
			{		
			   if(operationID==1)
				shiftMatrix[i][j] = part1_Invertible[k][l];
				else if(operationID==2)
				convolutionMatrix[i][j]= part1_Invertible[k][l];
			l++;
			}
			k++;
			l=0;
		}
		k=0; 
		l=0;
		
		for(i=matrixNewSize_2nX2n/2; i<matrixNewSize_2nX2n; ++i)
		{
			for(j=0; j<matrixNewSize_2nX2n/2; ++j)
			{
				if(operationID==1)
				shiftMatrix[i][j] = part2_Invertible[k][l];
				else if(operationID==2)
				convolutionMatrix[i][j]= part2_Invertible[k][l];
			l++;
			}
			k++;
			l=0;
		}
		k=0; 
		l=0;
		
		for(i=matrixNewSize_2nX2n/2; i<matrixNewSize_2nX2n; ++i)
		{
			for(j=matrixNewSize_2nX2n/2; j<matrixNewSize_2nX2n; ++j)
			{
				if(operationID==1)
				shiftMatrix[i][j] = part3_Invertible[k][l];
				else if(operationID==2)
				convolutionMatrix[i][j] = part3_Invertible[k][l];
			l++;	
			}
			k++;
			l=0;
		}
		k=0;
		l=0;	
			
			
		if(operationID==1)
		{
					fprintf(CLIENTLOGFILE,"\n");
					fprintf(CLIENTLOGFILE,"Original Matrix= ");
					fprintf(CLIENTLOGFILE,"[");

					for(k=0;k<matrixNewSize_2nX2n;k++)
					{
							for(l=0;l<matrixNewSize_2nX2n;l++)
							{
								fprintf(CLIENTLOGFILE,"%10.2f",inputMatrix[k][l]);	
							}
								fprintf(CLIENTLOGFILE,";");
					}
				fprintf(CLIENTLOGFILE,"]\n");	
			
				fprintf(CLIENTLOGFILE,"Shifted Inverse= ");
				fprintf(CLIENTLOGFILE,"[");	
				for(i=0; i<matrixNewSize_2nX2n; ++i)
				{
					for(j=0; j<matrixNewSize_2nX2n; ++j)
					{
				
						fprintf(CLIENTLOGFILE,"%10.2f",shiftMatrix[i][j]);
						copyShiftMatrix[i+1][j+1]=shiftMatrix[i][j];	
						copyInputMatrix[i+1][j+1]=inputMatrix[i][j];
					}
			
					fprintf(CLIENTLOGFILE,";");
				}
				fprintf(CLIENTLOGFILE,"]\n");
		
				
				
		}
		
		else if(operationID==2)
		{
				
				fprintf(CLIENTLOGFILE,"Convolution Matrix= ");
				fprintf(CLIENTLOGFILE,"[");	
				for(i=0; i<matrixNewSize_2nX2n; ++i)
				{
					for(j=0; j<matrixNewSize_2nX2n; ++j)
					{
				
						fprintf(CLIENTLOGFILE,"%10.2f",convolutionMatrix[i][j]);
						copyConvolutionMatrix[i+1][j+1]=convolutionMatrix[i][j];	
						copyInputMatrix[i+1][j+1]=inputMatrix[i][j];
					}
			
					fprintf(CLIENTLOGFILE,";");
				}
				fprintf(CLIENTLOGFILE,"]\n");		
		
		}
		
		if(operationID == 1)
		{
			result1 = computeDeterminant(copyInputMatrix,matrixNewSize_2nX2n) - computeDeterminant(copyShiftMatrix,matrixNewSize_2nX2n);
			return result1;
		}
		else 
		{
			result2 = computeDeterminant(copyInputMatrix,matrixNewSize_2nX2n) - computeDeterminant(copyConvolutionMatrix,matrixNewSize_2nX2n);
			return result2;
		}
}





int main(int argc, char **argv)
{
		int i,k,l;
		int allMatrixElementNumber=0,matrixIndex=0;
		int sizeMatrix=0;
	
		double result_1,result_2;
		int flagTimerServer=0;
		int pid,pid2;
	
		long double elapsTime_Result1,elapsTime_Result2;
		
		if (argc != 2)
		{
			usage();
			exit(0);
		}
		
			
		strcpy(MAINFIFO, argv[1]);

		memset(bufferedMatrixElements, 0, sizeof bufferedMatrixElements);
		memset(copyBufferedMatrixElements, 0, sizeof copyBufferedMatrixElements);
		
		if((fileDescMainFifo=open(MAINFIFO, O_WRONLY)) == -1)
		{
			printf("No such pid-server:%s\n", MAINFIFO);
			exit(0);
		}
		
		char pName[MAX_BUF];
		memset(pName,' ',MAX_BUF);
		strcpy(pName,"1");
		
		snprintf(pName+1,MAX_BUF-1,"%10ld",(long)getpid());
		

		write(fileDescMainFifo,pName,MAX_BUF);
		close(fileDescMainFifo);
	
		char pidTemp[10];
		snprintf(pidTemp,10,"%ld",(long)getpid());
		snprintf(CONNECTED_CLIENT_FIFO,20,"cli%ld",(long)getpid());
			
		
		mkfifo(CONNECTED_CLIENT_FIFO, 0666);
		
		fileDescConnectedClient = open(CONNECTED_CLIENT_FIFO,O_RDONLY);		
			
			 
		mkfifo(clientToResult_FIFO, 0666);
		clientToResult_ID = open(clientToResult_FIFO, O_WRONLY);

	
		if ((CLIENTLOGFILE=fopen(clientLogFile,"a")) == NULL)
		{
			printf("%s logfile couln't opened !\n", clientLogFile );
			exit(0);
		}

		
		while(1){
			
			while(read(fileDescConnectedClient,BUF,MAX_BUF) > 0 ){
					
				signal(SIGINT, &sigHandler);

				if(flagTimerServer==0)
				{
					timerServerPID=atof(BUF);
					fprintf(stderr,"Pid %ld", timerServerPID);
					flagTimerServer++;
				}
				
				if(atof(BUF)<1000)
				{
					 bufferedMatrixElements[allMatrixElementNumber] = atof(BUF);
					 allMatrixElementNumber++;
				}
				
				else 
				{
					sizeMatrix = atoi(BUF)-1000;
				}			
			
				if((sizeMatrix*sizeMatrix)==allMatrixElementNumber)
				{

					for(k=0;k<sizeMatrix;k++){
						for(l=0;l<sizeMatrix;l++) {
							copyBufferedMatrixElements[k][l]=bufferedMatrixElements[matrixIndex++];
							
						}
					}
					
					allMatrixElementNumber=0;
					matrixIndex=0;
				
					if ((pid = fork()) == -1)
					{
						perror("Fork Error !\n");
						exit(0);
					}
				
					else if(pid==0) {
				
					processPID = (long)getpid();
					fprintf(stderr,"Process pid :%ld\n", processPID);
					snprintf(BUF_CLIENT, MAX_BUF, "%ld",processPID);	
					write(clientToResult_ID, BUF_CLIENT, MAX_BUF);	
					

					// Time Elapse
					gettimeofday (&timeSpecBegin_1, NULL);

					
					result_1 = divideMatrix_nXn(1, copyBufferedMatrixElements, sizeMatrix);
					
					gettimeofday (&timeSpecEnd_1, NULL);
					
						elapsTime_Result1=((timeSpecEnd_1.tv_sec - timeSpecBegin_1.tv_sec)*1000 +
									(timeSpecEnd_1.tv_usec - timeSpecBegin_1.tv_usec )/1000 ); 
					
					//fprintf(stderr,"%ld\n", (long)(elapsTime_Result1));
				
					snprintf(BUF_CLIENT, MAX_BUF, "%f",result_1);	
					write(clientToResult_ID, BUF_CLIENT, MAX_BUF);
			
			
					snprintf(BUF_CLIENT, MAX_BUF, "%ld",(long)(elapsTime_Result1));	
					write(clientToResult_ID, BUF_CLIENT, MAX_BUF);
			
					// Time Elapse
			
					gettimeofday (&timeSpecBegin_2, NULL);
					
					result_2 = divideMatrix_nXn(2, copyBufferedMatrixElements, sizeMatrix);
					
					gettimeofday (&timeSpecEnd_2, NULL);
					
					elapsTime_Result2=((timeSpecEnd_2.tv_sec - timeSpecBegin_2.tv_sec)*1000 +
									(timeSpecEnd_2.tv_usec - timeSpecBegin_2.tv_usec )/1000 ); 
						
					//fprintf(stderr,"%ld--\n",(long)(elapsTime_Result2));
						
					snprintf(BUF_CLIENT, MAX_BUF, "%f",result_2);		
				    write(clientToResult_ID, BUF_CLIENT, MAX_BUF);
						
						
					snprintf(BUF_CLIENT, MAX_BUF, "%ld",(long)(elapsTime_Result2));	
					write(clientToResult_ID, BUF_CLIENT, MAX_BUF);
					

					unlink(clientToResult_FIFO);
					exit(0);
					}

					else {	
						wait(NULL);
						
					}
				
				}
			
				if(!strncmp(BUF,"FINISHED",8)){
						printf("Process is completed [%s]\n", BUF);
						close(fileDescConnectedClient);
						unlink(CONNECTED_CLIENT_FIFO);
						fclose(CLIENTLOGFILE);
						exit(1);
					}
			}
			
			
		}
		
return 0;
}

