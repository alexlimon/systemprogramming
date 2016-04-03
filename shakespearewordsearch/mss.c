/*
 * Name: Alex Limon	
 * ID #: 1000818599
 * Programming Assignment 2
 * Description: Shakespeare word countz
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

int main( void ) 
{ 
  pid_t pids[100];
  int spfd; // file descriptor for our file
  int comm[100][2];
  int long totalbytes;
  int i,totalhits=0,isSpace=0, temphits,readbuffer, querysize, workers, beginningbyte, endingbyte, currentworker=0, status=0 ;
  struct stat file_stats;// structure that contains the file size
  char *searchp; // a pointer to that memory
  char **input,*rawi,*temptok;
  struct timeval checkstart,checkend;  
  float totaltime;
 
  printf("Welcome to the Shakespeare word count service.\n");        //Basic output for the application
  printf("Enter: Search [word] [workers] to start your search.\n");
 
  
  while(1)
  {
    rawi = (char*)malloc(sizeof(char)*50);    // lets allocate memory for the raw input and pray the user doesnt use more than 50 chars
    input = (char**)malloc(sizeof(char*)*4);  // we need 3 parameters, but add one just in case
    for(i=0;i<3;i++) input[i]= (char*)malloc(sizeof(char)*30); // every argument can be at most 30 characters, because, why not?
     
    /* The following three lines reset the blank space flag, the totaltime a query took and i for allocating memory*/ 
    i = 0;              
    isSpace = 0;
    totaltime = 0;
    
    /* The next two lines are polling the user and grabbing any raw input */
    printf("> ");
    fgets(rawi,50,stdin);
    
    gettimeofday(&checkstart,NULL); // this will set the time the query started because its the moment we get the user input
   
    while( (temptok = strsep(&rawi," \n\r")) != NULL ) //take out the input very nicely 
    { 
      if( strstr(" ",temptok) == NULL ) // just in case strsep threw some random space as a token
      {
        input[i] = strndup(temptok, 255);  // now its safe to build our parameter array
        i++;
      }
      if(i==0) isSpace=1; // if we never entered the parameter array, it must be a space!
      if(i==3) break; // if we entered the parameter array too much, then we already have enough parameters
    }
    if(isSpace) continue; //start over, its just a space.. calm down
    i=0; // lets reset i 
   
    while( input[0][i] ) // converting search into uppercase just incase there is a problem 
    {
      input[0][i]= toupper(input[0][i]);
      i++;
    } 
   
    



    if( strcmp(input[0],"SEARCH") == 0) //this if statement is the meat of the program, we are checking for a possible search
    { 
         
       if( atoi(input[2]) > 100 || atoi(input[2]) <= 0) // first see if the user went crazy with the workers
       {
         printf("Please enter a valid number of workers or refer to 'help'. \n");
         continue;
       }
    
       if( strstr(" ", input[1]) != NULL)  // now lets see if the user actually put a word in there
       {
         printf("Please enter a valid word or refer to 'help'. \n");
         continue;
       }
    

       workers= atoi(input[2]); // now that its safe to use the worker count, convert it into an integer
       
       querysize = strlen(input[1]); // this is the length of the word being search aka query

       if(( spfd = open("shakespeare.txt",O_RDONLY)) < 0 ) // we need to open the file, but only in the parent, dont need to spam open with each wker
       {
         perror("open");
         exit(1);
       }

       if((fstat(spfd, &file_stats)) < 0) // lets grab some status on our file to see its size
       {
         perror("stat");
         close(spfd);
         exit(1);
       }
       
       totalbytes = file_stats.st_size; // the total size of the file, for further calculations
       

       for( currentworker=0; currentworker < workers; currentworker++) // lets iterate through all the workers that the user wants
       {
         
         temphits = 0;// reset the number of times we find the word for the next process
         
         if ( pipe(comm[currentworker])  == -1) // lets use our pipe and check if it failed
         { 
           perror("pipe"); 
           exit(1); 
         }
         
         pids[currentworker] = fork(); // pipe seems to be fine, lets create a worker/process
         
         if(pids[currentworker] == 0) // going into the child, bye dad!
         { 
           close(comm[currentworker][0]); // we don't need to ever read from a child
          
           /* logic behind this is simple, we need a ratio for the worker we are at, and based on that even ratio we see how many bytes are used */ 
           beginningbyte = ( currentworker / workers ) * totalbytes; 
          
           endingbyte = ( ( currentworker + 1 ) / workers ) * totalbytes;  // with the ending byte its basically where the next worker begins
           
           searchp = mmap((caddr_t)0, file_stats.st_size, PROT_READ, MAP_SHARED, spfd, 0); // lets create a mapping for our worker
           
           if( searchp == (caddr_t)(-1) ) //did our mapping work properly?
           {
             perror("mmap");
             exit(1);
           }
      
 
           for( beginningbyte; beginningbyte < endingbyte ; beginningbyte++ ) //finally the actually search!
           {
             /*we are gonna use beginningbyte as the offset and compare a byte at a time to our query, a 0 indicates we found it! */
             if( memcmp( searchp + beginningbyte , input[1] , querysize ) == 0 ) temphits++; 
             
           }
            write( comm[currentworker][1], &temphits, sizeof(int) ); // we want to write to the pipe how many times we found it
            exit(0);// kill the worker
         } 
      
       
      }

      for( currentworker = 0; currentworker < workers; currentworker++ )
      {
         
           close( comm[currentworker][1] ); // close the writing pipe, we only need to read
           read( comm[currentworker][0], &readbuffer, sizeof(int) ); // read what the worker reported as found
           totalhits+=readbuffer; // lets sum up what all our workers told us
      }

 
      gettimeofday(&checkend,NULL); // the query is done! lets stop time
     
     /* We are gonna check the what the beginning time was and what the ending time was and subtract them to get the querytime */
      totaltime = ( (checkend.tv_sec * 1000000) - (checkend.tv_sec *1000000) ) + ( checkend.tv_usec - checkstart.tv_usec);
    
     /*Use the OG string to show what we look for and OF COURSE the result from the parent pipes, and then the time above */
      printf( "Found %d instances of %s in %f microseconds.\n " , totalhits, input[1] , totaltime );
      totalhits=0; //reset the number of words we found for the next query, no huge sums please!
     
   }
   else if( strcmp(input[0], "HELP") == 0) // check for the help command and give the user what he needs to know
   {
     printf("\nShakespeare Word Search Service Command Help\n");
     printf("______________________________________________ \n");
     printf("\n");
     printf("help   - displays this message\n");
     printf("quit   - exits\n");
     printf("search [word] [workers] - searches the works of Shakespeare for [word] using\n");
     printf("                          [workers].  [workers] can be from 1 to 100.\n");
   }
   else if( strcmp(input[0], "QUIT") == 0) // let the user quit
   {
    exit(0);
   }
   else //huh?? what command did you use again? it definitely wasn't a space
   {
      printf("Invalid command. Try again.\n");
   }
 }



 return 0 ;
}
