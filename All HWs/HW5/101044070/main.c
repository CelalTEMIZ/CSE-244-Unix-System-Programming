
//
//
// CELAL TEMIZ
// 101044070
// SYSTEM PROGRAMMING HW_5
//
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>


FILE* outp;
FILE* tsifp;
FILE* tnolp;
FILE* nstcp;
FILE* parentReadTempFile, *parentWriteTempFile;


#define OUTPUT_FILE_NAME "log.log" 
#define TOTAL_STRING_IN_FILE "tsif.txt"
#define TOTAL_LINE_NUMBER_IN_FILE "tnol.txt"
#define TOTAL_NUMBER_SEARCH_THREADS "nstc.txt"
#define PATH_MAX 4096    /* # chars in a path name including nul */
#define CHARACTER_SLASH "/"
#define FILE_TYPE "txt"
#define TEMP_FILE "temp.txt"


static int numberOfDirectories=0 ; 
static int numberOfFiles=0;
static int lineNumberInAllFiles=1;
static int numberOfSearchThreadsCreated=0;


static int numberOfSharedMemory=0;

int exitConditionFlag=1;
int signalFlag = 0; 
pthread_t signalThreadID;


sem_t mutex; // SEMAPHORE

int   fileType(char *path);
int   searchDirectory(char *path, char *string);
int   totalNumberOfString(FILE* outputFILE, char **argv);
int   totalNumberOfLine();
int   totalNumberOfThreads();
bool  isTextFile(char *fileName);
void* signalThread(void*);
void* grepTh(void*);


// Message queue data blog
// The msgsz argument specifies the length of the message in bytes.
// Reference : https://users.cs.cf.ac.uk/Dave.Marshall/C/node25.html

typedef struct
{
    long    mtype;          // Message Type
    char    mtext[1028];    // Message Text of length msgsz

}message_buf;


// Message queue to between all directories
message_buf sbuf_directory;

// Incoming data blog to threads processing
typedef struct{
    char filePath[PATH_MAX];
    char threadString[PATH_MAX];
    char threadName[PATH_MAX];
}thread_t;


int main(int argc, char *argv[])
{

    // Total run time start
    clock_t begin = clock();

    sigset_t set;

    outp = fopen(OUTPUT_FILE_NAME,"a");   
   
    if(argc != 3){
        fprintf(stderr, "Usage : %s <string> <folderName> \n", argv[0]);
        return -1;
    }

    // Set signal to thread
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    
  
    // Semaphore Initialize and create named signal thread to exit condition
    sem_init(&mutex, 0, 1);
    pthread_create( &signalThreadID, NULL, signalThread, NULL );


    // Content of the screen
    searchDirectory(argv[2], argv[1]);
  
    fprintf(stderr,"\nTotal number of strings found       : %d\n", totalNumberOfString(outp, argv));
    
    fprintf(stderr,"Number of directories searched      : %s\n", sbuf_directory.mtext );
 
    fprintf(stderr,"Number of files searched            : %d\n", numberOfFiles );

    fprintf(stderr,"Number of lines searched            : %d\n", totalNumberOfLine() );

    fprintf(stderr,"Number of cascade threads created   : %s\n", "XX" );

    fprintf(stderr,"Number of search threads created    : %d\n", totalNumberOfThreads() );

    fprintf(stderr,"Max of threads running concurrently : %s\n", "XX" );

    fprintf(stderr,"Number of shared memory created     : %d\n", numberOfSharedMemory - 1 );           // -1 Main Folder

    fclose(outp);


    // Total run time end 
    clock_t end = clock();
    double msec = (double)(end - begin) * 1000 / CLOCKS_PER_SEC;

    
    fprintf(stderr,"Total  run time in miliseconds      : %.2f\n", msec);

    if(exitConditionFlag == 1) {
    
        fprintf(stderr,"Exit   condition \t\t    : %s\n\n", "Normal");
    }

    if(exitConditionFlag == 2) {

        fprintf(stderr, "Exit   Contion \t\t\t    : Due to unknown file \n\n" );

    }
    
    if (exitConditionFlag == 3) {
        fprintf(stderr, "Exit   Contion \t\t\t    : Due to Ctrl-C Signal \n\n" );
    }

    unlink(TEMP_FILE);
    return 0;
}


// Exit Condition CTRL+C signal
void* signalThread(void* signalThreadValue)
{

   int sig;
   sigset_t set;
  
   sigemptyset(&set);
   sigaddset(&set,SIGINT);
   pthread_detach(pthread_self());
   pthread_sigmask(SIG_BLOCK,&set,NULL);
   
   sigwait(&set,&sig);
   signalFlag=1;

   exitConditionFlag=3;
   pthread_exit(NULL);
}

// Return file types (Files or directories)
int fileType(char *path)
{
    
    struct stat sb;

    if (stat(path, &sb) == -1) {
        fprintf(stderr, "stat : %s\n", path);
        return -1;
    }

    return sb.st_mode & S_IFMT;
}

// Search all directories in any given path

int searchDirectory(char *path, char *string)
{

    int numOfFiles=0,i=0,status=0;
    char fullPath[_POSIX_PATH_MAX],resultFilePath[PATH_MAX],logFilePath[PATH_MAX];
    char *pos,*buffer;
    long logFileSize;

    struct dirent **nameList;
    thread_t   threadInfo;
    pid_t processToThreadPID; 
    pthread_t  grepThID;
  

    // Shared Memory Objects to all directories 
    // Shared Memory Variables 
    int shmid;
    char* shm;
    const int shm_size = 1024;
    key_t key = 5678;
    /// Shared Memory ///




    // Message Queue between all directores
    // Message Queue variables
    char str[1024];
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key_msg;
    size_t buf_length;

    // Get the message queue id for the "name" 9876, which was created by the server.
     
    key_msg = 9876; 

    if ((msqid = msgget(key_msg, msgflg )) < 0){   
        perror("msgget");
        exit(1);
    }
    ////////////// Message Queue  Operations End /////////////////////




    ////////////// SHared memory OPerations  //////////////////////

    // We'll name our shared memory segment "5678". 
    // Create the segment.

    if ((shmid = shmget(key, shm_size, IPC_CREAT | S_IRUSR | S_IWUSR)) < 0){
        perror("shmget");
        return 1;   
    }

    ++numberOfSharedMemory;

    
    // Now we attach the segment to our data space.

    if ((shm = shmat(shmid, NULL, 0)) < 0) {
        perror("shmat");
        return 1;
    }



    ////////////////////// Shared Memory Operations End ////////////////////


    nstcp = fopen(TOTAL_NUMBER_SEARCH_THREADS, "a");

    // http://stackoverflow.com/questions/18402428/how-to-properly-use-scandir-in-c

    numOfFiles = scandir(path, &nameList, NULL, alphasort);

    // Number of files and create thread amount of number of files

    if(numOfFiles < 0)
        perror("scandir");

        if(numOfFiles > 2) 
        {
                for( i = 0; i < numOfFiles; ++i )
                {
                    if(signalFlag)
                        break;
     
                    if( !strcmp(nameList[i]->d_name , "." ) || !strcmp(nameList[i]->d_name , ".." )  ){
                        continue;
                    }

                    strcpy(fullPath, path); 
                    strcat(fullPath, CHARACTER_SLASH);
                    strcat(fullPath, nameList[i]->d_name);

                    int type = fileType(fullPath);

                    if(type == S_IFDIR){

                        ++numberOfDirectories;

                            sbuf_directory.mtype = 1;
                            sprintf( sbuf_directory.mtext,"%d", numberOfDirectories);
                            
                            // Send message.
                            if (msgsnd(msqid, &sbuf_directory, buf_length, IPC_NOWAIT) < 0)
                            {
                                //printf ("%d, %ld, %s, %lu\n", msqid, sbuf_directory.mtype, sbuf_directory.mtext, buf_length); 
                                perror("msgsnd");
                                exit(1);
                            }

                            // Print message queue data to see
                            // printf ("%s\n", sbuf_directory.mtext);

                     
                            // Recursive Call to nested directories
                            searchDirectory(fullPath, string);      
                    }

                    // Create threads for each file 

                    else if(type == S_IFREG)
                    {
                        ++numberOfFiles;
                        processToThreadPID=fork();

                        if(processToThreadPID == 0)
                        {
                            sprintf(threadInfo.filePath,"%s",fullPath);
                            sprintf(threadInfo.threadString,"%s",string);
                            sprintf(threadInfo.threadName,"%s",path);
                        
                            if(pthread_create(&grepThID,NULL,(void*)grepTh, &threadInfo )) {
                                fprintf(stderr, "Error creating thread\n");
                                return 1;
                            }
 
                            numberOfSearchThreadsCreated++;
                            fprintf(nstcp, "%d\n", numberOfSearchThreadsCreated );
                           
                            if(pthread_join(grepThID, NULL)) {

                                fprintf(stderr, "Error joining thread\n");
                                return 2;

                            }

                            exit(0);
                        }


                        while ((processToThreadPID = wait(&status)) > 0)
                        {
                           
                            sprintf(logFilePath, "%s%s%d.%s", path, CHARACTER_SLASH, processToThreadPID, FILE_TYPE);
                            parentReadTempFile = fopen(logFilePath, "rb");

                            sprintf(resultFilePath, "%s%s%s", path, CHARACTER_SLASH, TEMP_FILE);    
                            parentWriteTempFile = fopen(resultFilePath, "ab");

                            fseek(parentReadTempFile, 0, SEEK_END);
                            logFileSize = ftell(parentReadTempFile);

                            rewind(parentReadTempFile);
                            
                            buffer = (char *)malloc(sizeof(char)*logFileSize);
                            fread(buffer,sizeof(char),logFileSize,parentReadTempFile);
                            fwrite(buffer,sizeof(char),logFileSize,parentWriteTempFile);
                            
                            free(buffer);

                            fclose(parentReadTempFile);
                            fclose(parentWriteTempFile);

                            remove(logFilePath);
                        }
                    }

                }

                
                while(--numOfFiles)
                {
                    free(nameList[numOfFiles]);
                }

                free(nameList);

                sprintf(logFilePath, "%s%s%s", path, CHARACTER_SLASH, TEMP_FILE);
                strcpy(resultFilePath, path);
                
                pos = strrchr(resultFilePath, CHARACTER_SLASH[0]);

                if(pos != NULL)
                {

                    pos[0] = '\0';
                    sprintf(resultFilePath, "%s%s%s", resultFilePath, CHARACTER_SLASH, TEMP_FILE);

                    parentReadTempFile = fopen(logFilePath, "rb");
                    parentWriteTempFile = fopen(resultFilePath, "ab");
                    fseek(parentReadTempFile, 0, SEEK_END);
                    logFileSize = ftell(parentReadTempFile);
                    rewind(parentReadTempFile);
                            
                    buffer = (char *)malloc(sizeof(char) * logFileSize);
                    fread(buffer, sizeof(char), logFileSize, parentReadTempFile);
                    fwrite(buffer, sizeof(char), logFileSize, parentWriteTempFile);

                    fprintf(parentWriteTempFile, "%s\n", "Ctrl-C signal handled !...");
                   
                    free(buffer);

                    fclose(parentReadTempFile);
                    fclose(parentWriteTempFile);

                    remove(logFilePath);

                }

 
      /* Detach the shared memory segment.  */ 
    if(shmdt(&shmid) != 0)
        fprintf( stderr," ");

        }
    

    return 0;

}

//
// Reference to message queue  
// https://users.cs.cf.ac.uk/Dave.Marshall/C/node25.html
//

void* grepTh(void* arg)
{

    int  sumOfString = 0;
    int  sumOfLines=0, lineNumber=1 ;
    int  reset=0;
    int  input_status=0;
    char ch, nextChar;
    int  readCharNumber=0;
    int  eoff=0;
    long size;
    int  char_status=0;
    int  sameCharacter=0;
    int  stringSize;

    thread_t* threadData = (thread_t*)arg ;

    FILE *inp, *outpTempFile;
    int i=0;
    char logFilename[_POSIX_PATH_MAX];
  


    int segment_id; 
    char* shared_memory; 
    struct shmid_ds shmbuffer; 
    int segment_size; 
    const int shared_segment_size = 0x6400; 
 
    char str[8192];

    // Allocate a shared memory segment
    segment_id = shmget (IPC_PRIVATE, shared_segment_size, 
                     IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); 
 
    // Attach the shared memory segment
    shared_memory = (char*) shmat (segment_id, 0, 0); 
    //printf ("shared memory attached at address %p\n", shared_memory); 
  
    // Determine the segment's size. 
    shmctl (segment_id, IPC_STAT, &shmbuffer); 

    segment_size  = shmbuffer.shm_segsz; 
    
    //printf ("segment size: %d\n", segment_size); 
 
  
   
    // Open program log file
    outp   = fopen(OUTPUT_FILE_NAME, "a");

    // Open temp output files to calculate sum of strings and line numbers
    tsifp  = fopen(TOTAL_STRING_IN_FILE,"a");
    tnolp  = fopen(TOTAL_LINE_NUMBER_IN_FILE,"a");


    inp = fopen( threadData->filePath , "r");   

    if(inp == NULL){
        fprintf(stderr, "File couldn't opened\n");
        exit(-1);
    }


    sprintf(logFilename, "%s%s%d.%s", threadData->threadName , CHARACTER_SLASH, (int)getpid(), FILE_TYPE);
    outpTempFile = fopen(logFilename, "w+");

    // Beginning of the critical region
    // Lock Semaphore

    sem_wait(&mutex); 


    fprintf(outpTempFile, "%s\n", threadData->filePath);

   
    stringSize = strlen(threadData->threadString);

    input_status = fscanf(inp, "%c", &ch);  
    ++readCharNumber;

    while(input_status != EOF && !eoff )
    {

        if(ch == threadData->threadString[0])
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
                            if(nextChar == threadData->threadString[i])
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

        // fprintf(stderr,"ProcessID: %d- ThreadID: %s:   [%d, %d] %s first character is found.\n",
               // getpid(), threadData->filePath, lineNumber, readCharNumber, threadData->threadString);

    
             /////////////////////////////////////////////////////////////////////////////////////////////////
             //                                                                                             //
             //                    Write data string to the shared memory segment                           //
             //                                                                                             //
             // //////////////////////////////////////////////////////////////////////////////////////////////

             sprintf(str, "ProcessID: %d- ThreadID: %s:   [%d, %d] %s first character is found.\n",
                    getpid(), threadData->filePath, lineNumber, readCharNumber, threadData->threadString );

            

            // Write log file from Shared Memory to Output File
             fprintf(outp, "%s", str);

             sprintf (shared_memory, "%s" , str); 

            // Detach the shared memory segment.  
             shmdt (shared_memory); 


            // Reattach the shared memory segment, at a different address.   
             shared_memory = (char*) shmat (segment_id, (void*) 0x5000000, 0); 

            // Print out the string from shared memory.   
            //  printf ("%s\n", shared_memory); 


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
            sumOfLines +=1;


        }
        if(eoff!=1)
            input_status=fscanf(inp, "%c", &ch);
            ++readCharNumber;
     }



    // Detach the shared memory segment.  
    shmdt (shared_memory); 
             
    // Deallocate the shared memory segment.  
    shmctl (segment_id, IPC_RMID, 0); 


     // Write changing values to temp output files
     // And delete them end of the computation
     fprintf(tsifp, "%d\n", sumOfString);
     fprintf(tnolp, "%d\n", sumOfLines );

    // Unlock semaphore to another thread 
    sem_post(&mutex);  

   
    // Close opened files
    fclose(inp);
    fclose(outp);
    fclose(tsifp);


}


//  Number of strings in all input file
int totalNumberOfString(FILE *outputFILE, char **argv)
{
    tsifp = fopen(TOTAL_STRING_IN_FILE,"r");

    int sum=0,result=0;

    while(fscanf(tsifp,"%d", &sum)!=EOF){
        result += sum;
    }

    fclose(tsifp);

    fprintf(outputFILE, "\n%d %s were found in total. \n", result, argv[1]);

    unlink(TOTAL_STRING_IN_FILE);


    return result;
}

bool isTextFile(char *fileName)
{
    size_t len = strlen(fileName);
    return len > 4 && strcmp(fileName + len - 4, ".txt") == 0;
}


int totalNumberOfLine()
{

    tnolp = fopen(TOTAL_LINE_NUMBER_IN_FILE, "r");

    int sumLines=0,resultLines=0;

    while(fscanf(tnolp, "%d", &sumLines) != EOF) {

        resultLines += sumLines;
        
    }

    fclose(tnolp);
    
    unlink(TOTAL_LINE_NUMBER_IN_FILE);

    return resultLines;


}


int totalNumberOfThreads()
{

    nstcp = fopen(TOTAL_NUMBER_SEARCH_THREADS, "r");

    int sumThreads=0,resultThreads=0;

    while(fscanf(nstcp, "%d", &sumThreads) != EOF) {

        resultThreads += sumThreads;
        
    }

    fclose(nstcp);
    
    unlink(TOTAL_NUMBER_SEARCH_THREADS);

    return resultThreads;


}
