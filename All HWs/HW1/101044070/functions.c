#include <stdio.h>
#include <stdlib.h>

#include "functions.h"


void numberOfStrings(FILE* inp, char *string, int stringSize) 
{
		
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
			   
 			    	printf("\n[%d, %d] konumunda ilk karakter bulundu.\n", lineNumber, readCharNumber );

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

	printf("\n%d adet %s bulundu.\n", sumOfString, string );

	
}



