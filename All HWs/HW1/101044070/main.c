
// CELAL TEMIZ
// 101044070
// SYSTEM PROGRAMMING HW_1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functions.h"


int main(int argc, char **argv) 
{
      
	    FILE *inputFile;
	 
	    int  total=0, i;
	  
	    int  stringSize=0;	    
			
        if(argc != 3) {        
            printf("Program Usage : ./list string file\n");
            exit(1);
        }
    
		inputFile = fopen(argv[2], "r");		

		if (inputFile == NULL ) {				
		   perror("Input File Couldn't Opened");
		}
				
		stringSize = strlen(argv[1]);
		

		// SEARCH TARGET STRING						
		numberOfStrings(inputFile,argv[1], stringSize);	

		fclose(inputFile);

	return 0;

}
