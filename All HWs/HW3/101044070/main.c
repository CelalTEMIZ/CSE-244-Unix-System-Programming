//
//
// CELAL TEMIZ
// 101044070
// SYSTEM PROGRAMMING HW_3
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

#define OUTPUT_FILE_NAME "log.log"
#define TOTAL_STRING_IN_FILE "tsif.txt"

#define BUFSIZE 4096
#define FIFO_PERM (S_IRUSR | S_IWUSR)

FILE* outp;
FILE* tsifp;

//Functions

void  numberOfStrings(char* readFileName, char *string, int fd[2]);
int   readDirectory(char *directoryArg, char *keyString, char *fifoName);
int   totalNumberOfString();


int main(int argc, char **argv) 
{
    
        outp = fopen(OUTPUT_FILE_NAME,"a");   

        if(argc != 3) {        
            fprintf(stderr,"Program Usage : %s string dirName\n", argv[0]);
            exit(1);
        }

        // argv[1] = string
        // argv[2] = dirName
        readDirectory(argv[2], argv[1], "myfifo");

        fprintf(outp,"\n%d %s were found in total.\n", totalNumberOfString(), argv[1]);

        fclose(outp);

    return 0;

}


//
// Reference    
// http://www.linuxquestions.org/questions/programming-9/d_type-in-struct-dirent-374/
//

int readDirectory(char *directoryArg, char *keyString, char *fifoName)
{

    struct stat stDirInfo;
    struct dirent * stFiles;
    DIR * stDirIn;
    char szFullName[MAXPATHLEN]; 
    char szDirectory[MAXPATHLEN];
    struct stat stFileInfo;

    pid_t childpid;

    // Pipe Requirements

    int fd[2]; 
    char readBuffer[BUFSIZE];

    strncpy(szDirectory, directoryArg, MAXPATHLEN - 1 );
    

    // FIFO for directories 

    if (mkfifo(fifoName, FIFO_PERM) == -1) { /* create a named pipe */
        if (errno != EEXIST) {
            fprintf(stderr, "[%ld]:failed to create named pipe %s: %s\n",
                            (long)getpid(), fifoName, strerror(errno));
            return -1;
        }
    }

  
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
            
                readDirectory(szFullName, keyString, fifoName);  

            }
        }
        else 
        {
            // Pipe for files and directories

            if (pipe(fd) == -1)
            {
                perror("Failed to create the pipe");
                exit(EXIT_FAILURE);
            }
            
            // Create Processes

            childpid = fork(); 


            if(childpid == -1)
            {
                perror("Failed to create the fork"); 
                exit(1);
            }

            // Child Process
            // Search given string in files and send result strings to the pipe 

            if (childpid == 0) 
            {   

                numberOfStrings(szFullName, keyString, fd);
                exit(1); 

            // Parent Process
            // Parent reads what its child has written to pipe

            }
            else
            { 
                // Parent process closes up output side of pipe
                close(fd[1]);
                while(read(fd[0], readBuffer, BUFSIZE) > 0) {
                      //Print pipe content to user
                      printf("Received string: %s", readBuffer);
                }
                // Wait Child Processes
                wait(NULL); 
            }
        }

    }  

    while ((closedir(stDirIn) == -1) && (errno == EINTR)) ;

    unlink(fifoName);
    
    return 0;
}

// Function to calculate number of strings in all input files
void numberOfStrings(char* readFileName, char *string, int fd[2]) 
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

    // To Write Operation
    char buf[BUFSIZE]; 

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
                    // Write to pipe
                    sprintf(buf,"%s:[%d, %d] %s first character is found.\n", readFileName, lineNumber, readCharNumber, string);
                    // Child process closes up input side of pipe 
                    close(fd[0]);
                    write(fd[1], buf, BUFSIZE ); 
                    fprintf(outp,"%s:   [%d, %d] %s first character is found.\n", readFileName, lineNumber, readCharNumber, string);
                    
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
    // Write to pipe
    sprintf(buf,"Total string number: %d \n",sumOfString);
    close(fd[0]);
    write(fd[1], buf, BUFSIZE );

    // Close opened files
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
