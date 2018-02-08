//
//
// CELAL TEMIZ
// 101044070
// SYSTEM PROGRAMMING HW_2
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>


#define OUTPUT_FILE_NAME "log.log"
#define TOTAL_STRING_IN_FILE "tsif.txt"


FILE* outp;
FILE* tsifp;

//Functions

void  numberOfStrings(char* readFileName, char *string);
int   readDirectory(char *directoryArg, char *keyString);
int   totalNumberOfString();


int main(int argc, char **argv) 
{
      int occurenceNumber=0;
		

		outp=fopen(OUTPUT_FILE_NAME,"a");	

        if(argc != 3) {        
            printf("Program Usage : ./listdir string dirName\n");
            exit(1);
        }

		// argv[1] = string,	argv[2] = dirName
		readDirectory(argv[2], argv[1]);

		occurenceNumber=totalNumberOfString();

		fprintf(outp,"\n%d %s were found in total.\n", occurenceNumber, argv[1]);

		fclose(outp);

	return 0;

}


//
// Reference	
// http://www.linuxquestions.org/questions/programming-9/d_type-in-struct-dirent-374/
//

int readDirectory(char *directoryArg, char *keyString)
{
    struct stat stDirInfo;
    struct dirent * stFiles;
    DIR * stDirIn;
    pid_t childpid;

    char szFullName[MAXPATHLEN]; 
    char szDirectory[MAXPATHLEN];
    struct stat stFileInfo;

    strncpy(szDirectory, directoryArg, MAXPATHLEN - 1 );

  
    if (lstat( szDirectory, &stDirInfo) < 0)
    {
        perror (szDirectory);
        return -1;
    }

    if (!S_ISDIR(stDirInfo.st_mode))
        return -1;
    if ((stDirIn = opendir( szDirectory)) == NULL) 
    {
        perror( szDirectory );
        return -1;
    }

    while (( stFiles = readdir(stDirIn)) != NULL)
    {
        sprintf(szFullName, "%s/%s", szDirectory, stFiles -> d_name );

        if (lstat(szFullName, &stFileInfo) < 0)
           perror ( szFullName );

   
        if (S_ISDIR(stFileInfo.st_mode))
        {
            if ((strcmp(stFiles->d_name , "..")) && (strcmp(stFiles->d_name , ".")))
            {
               // Read Nested Directories 
                readDirectory(szFullName, keyString);
            }
        }
        else 
        {
        	// Create Processes

            childpid = fork(); 
          
           
            if(childpid == -1)
            {
                perror("Failed to fork");
      			return 1;
            }

           	// Child Process
            if (childpid == 0) 
            {
                // Call function
                numberOfStrings(szFullName, keyString);
                exit(1);
                //Kill child processes
            }
            else
            {
            	//Parent waits child processes
                wait(NULL); 
            }
        }

    }  
    
    while ((closedir(stDirIn) == -1) && (errno == EINTR)) ;

    return 0;
}

// Function to calculate number of strings in all input files
void numberOfStrings(char* readFileName, char *string) 
{
	FILE* inp;	

	int  input_status=0;
	int  sumOfString = 0;
	int  lineNumber = 1;
	int  i=0,j=0;
	long size;
	
	int readCharNumber=0;

	int reset=0;

	int  char_status=0;
	char ch, nextChar;
	int  sameCharacter=0;
	int  eoff=0;
	
	int stringSize = strlen(string);	


	inp  = fopen(readFileName,"r");
	outp = fopen(OUTPUT_FILE_NAME,"a");
	tsifp=fopen(TOTAL_STRING_IN_FILE,"a");

	// READ FIRST CHAR OF INPUT FILE
	input_status = fscanf(inp, "%c", &ch);	
	++readCharNumber;
	

	// READ FILE TO END OF THE FILE	
	while(input_status != EOF && !eoff){
	
		if(ch == string[0])
		{
		     sameCharacter=1;

		     // Get current position in stream
		     // Returns the current value of the position indicator of the stream
		     size = ftell(inp);
		   
		     for(i=1; i<stringSize; i++){
		     
		     	  char_status = fscanf(inp, "%c", &nextChar);		     	 	 

		     	  	// IGNORE SOME CHARACTERS
			          if(char_status != EOF ){
			          		if(nextChar == ' ' || nextChar == '\n' || nextChar == '\t'){
			          			--i;
			          			++reset;
			          		}	          
			                if(nextChar == string[i])
			          	       sameCharacter +=1;
			          	   
			          }
			          else{
			          	i=stringSize;
			          	eoff=1;

			          }

             }		  
		  	// IF CHARACTER NUMBER IS EQUAL STRING SIZE
            // THEN INCREMENT THE SUM OF STRING
		     if(sameCharacter == stringSize){
			    sumOfString +=1;
 			    	fprintf(outp,"%s:	[%d, %d] %s first character is found.\n", readFileName, lineNumber, readCharNumber, string);
 			    	
		     }
		  
		     // Sets the position indicator associated with the stream to a new position.
		     // Beginning of file
		     if(!eoff)
		     fseek(inp, size, SEEK_SET);
		       
		}		
		// COUNT LINE NUMBER
		if(ch == '\n') {		
			lineNumber +=1;
			readCharNumber=0;

		}
		if(eoff!=1)
		input_status=fscanf(inp, "%c", &ch);

		++readCharNumber;

	}

	fprintf(tsifp, "%d\n", sumOfString);


	fclose(inp);
	fclose(outp);
	fclose(tsifp);
	
}

//  Number of strings in all input file
int totalNumberOfString()
{
	tsifp = fopen(TOTAL_STRING_IN_FILE,"r");

	int sum=0,result=0;

	while(fscanf(tsifp,"%d", &sum)!=EOF){
	result += sum;
	}

	fclose(tsifp);
	unlink(TOTAL_STRING_IN_FILE);

	return result;
}
