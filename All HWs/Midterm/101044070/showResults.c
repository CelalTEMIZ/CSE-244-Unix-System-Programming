#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>


#define SHOWRESULTLOGFILE "showResult.txt"
#define MAX_BUF 4096


FILE *SHOWRESULT_LOGFILE;

char BUF_CLIENT[MAX_BUF];
int clientToResult_ID;

int main(int argc, char* argv[])
{

	int numberOfIncomingData=0;
 
    clientToResult_ID= open("clientToResult_FIFONAME", O_RDONLY);

    while(1){
        while(read(clientToResult_ID,BUF_CLIENT,MAX_BUF) > 0){
        	if(numberOfIncomingData	==	0){		
        	  	if((SHOWRESULT_LOGFILE=fopen(SHOWRESULTLOGFILE,"a")) != NULL){
					fprintf(SHOWRESULT_LOGFILE,"pid = %ld\n ",atol(BUF_CLIENT));
					fprintf(stderr,"pid = %ld",atol(BUF_CLIENT));
					fclose(SHOWRESULT_LOGFILE);
				}
					numberOfIncomingData++;	
			}
        	if(numberOfIncomingData == 1) {		
        	  	if((SHOWRESULT_LOGFILE=fopen(SHOWRESULTLOGFILE,"a")) != NULL){
					fprintf(SHOWRESULT_LOGFILE,"Result 1 = %f,", atof(BUF_CLIENT));
					fprintf(stderr,"Result 1 = %f,", atof(BUF_CLIENT));
					fclose(SHOWRESULT_LOGFILE);
				}
					numberOfIncomingData++;			
			}

        	if(numberOfIncomingData == 2){		
        	  	if((SHOWRESULT_LOGFILE=fopen(SHOWRESULTLOGFILE,"a")) != NULL){
					fprintf(SHOWRESULT_LOGFILE,"Time 1 = %f\n ",atof(BUF_CLIENT));
					fprintf(stderr,"Time 1  = %f,", atof(BUF_CLIENT));
					fclose(SHOWRESULT_LOGFILE);
				}
					numberOfIncomingData++;			
			}
			if(numberOfIncomingData == 3){
				if((SHOWRESULT_LOGFILE=fopen(SHOWRESULTLOGFILE,"a")) != NULL){
					fprintf(SHOWRESULT_LOGFILE,"Result 2= %f,",atof(BUF_CLIENT));
					fprintf(stderr,"Result 2 = %f\n ",atof(BUF_CLIENT));
					fclose(SHOWRESULT_LOGFILE);
				}
					numberOfIncomingData++;	
			}
        	if(numberOfIncomingData == 4){		
        	  	if((SHOWRESULT_LOGFILE=fopen(SHOWRESULTLOGFILE,"a")) != NULL){
					fprintf(SHOWRESULT_LOGFILE,"Time 2= %f\n ",atof(BUF_CLIENT));
					fprintf(stderr,"Time 1  = %f,", atof(BUF_CLIENT));
					fclose(SHOWRESULT_LOGFILE);
				}
				
			}
	
			numberOfIncomingData = 0;
        }

       
         break;
    }

    return 0;
}  

