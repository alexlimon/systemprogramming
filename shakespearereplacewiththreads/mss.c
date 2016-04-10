/*
 * * Name: Alex Limon	
 * * ID #: 1000818599
 * * Programming Assignment 3
 * * Description: Shakespeare word countz
 * */

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
#include <pthread.h>


void* threadSearch( int currentworker );
void* threadReplace( int currentworker );

pthread_mutex_t mutexa;
pid_t resetpid;
int spfd, querysize, workers, totalhits= 0;
int long totalbytes = 0;
char wordQ[31], wordR[31];
char* searchp;

int main( void )
{ 
  pthread_t threads[100];
  int rets[100];
  int i,isSpace=0, currentworker=0, status=0, wdiff ;
  struct stat file_stats;// structure that contains the file size
  char **input,*rawi,*temptok;
  struct timeval checkstart,checkend;  
  float totaltime;
 
  printf("Welcome to the Shakespeare word count service.\n");   //Basic output for the application
  printf("Enter: Search [word] [workers] to start your search.\n");
 
  
  while(1)
  {
    rawi = (char*)malloc(sizeof(char)*50);  // lets allocate memory for the raw input and pray the user doesnt use more than 50 chars
    input = (char**)malloc(sizeof(char*)*5); // we need 4 parameters, but add one just in case
    for(i=0;i<4;i++) input[i]= (char*)malloc(sizeof(char)*30); //every argument can be at most 30 characters, because, why not?
     
   
    i = 0;              
    isSpace = 0;
    totaltime = 0;
    
   
    printf("> ");
    fgets(rawi,50,stdin);// getting the user input
    
      
    while( (temptok = strsep(&rawi," \n\r")) != NULL ) //take out the input very nicely 
    { 
      if( strstr(" ",temptok) == NULL ) // just in case strsep threw some random space as a token
      {
        input[i] = strndup(temptok, 255);  // now its safe to build our parameter array
        i++;
      }
      if(i==0) isSpace=1; // if we never entered the parameter array, it must be a space!
      if(i==4) break; // if we entered the parameter array too much, then we already have enough parameters
    }
    if(isSpace) continue; //start over, its just a space.. calm down
    i=0; // lets reset i 
   
    while( input[0][i] ) // converting search into uppercase just incase there is a problem 
    {
      input[0][i]= toupper(input[0][i]);
      i++;
    } 
   
    



    if( strcmp(input[0],"SEARCH") == 0) //after parsing nicely we find search
    { 
         
       if( atoi(input[2]) > 100 || atoi(input[2]) <= 0) //exception handling for workers
       {
         printf("Please enter a valid number of workers or refer to 'help'. \n");
         continue;
       }
    
       if( strstr(" ", input[1]) != NULL) //exception handling for word
       {
         printf("Please enter a valid word or refer to 'help'. \n");
         continue;
       }
        
       gettimeofday(&checkstart,NULL); //starting query time

       workers= atoi(input[2]);
       querysize = strlen(input[1]);
       strcpy(wordQ, input[1]);
    
       if(( spfd = open("shakespeare.txt",O_RDONLY)) < 0 ) //opening the file we need to search
       {
         perror("open");
         exit(1);
       }

       if((fstat(spfd, &file_stats)) < 0)  //check for any errors with the fstat call
       {
         perror("stat");
         close(spfd);
         exit(1);
       }
       
       totalbytes = file_stats.st_size; //the totalsize of the file
        
       searchp = mmap((caddr_t)0, totalbytes, PROT_READ, MAP_SHARED, spfd, 0); //mapping the file only for reading
       
       if( searchp == (caddr_t)(-1) ) //did our mapping work properly?
       {
        perror("mmap");
        exit(1);
       }

       for( currentworker=0; currentworker < workers; currentworker++) //this is going to create the threads 
       {
         rets[currentworker] = pthread_create(&threads[currentworker], NULL, &threadSearch, currentworker);// keep return values for errors
         if ( rets[currentworker] != 0) perror("pthread_create");// error occured
       
       }
      
       
     

      for( currentworker = 0; currentworker < workers; currentworker++ )
      {
          pthread_join(threads[currentworker], NULL); //join the threads before main continues
      }

 
      gettimeofday(&checkend,NULL); // the query is done! lets stop time
     
    
      totaltime = ( (checkend.tv_sec * 1000000) - (checkend.tv_sec *1000000) ) + ( checkend.tv_usec - checkstart.tv_usec);
    
      printf( "Found %d instances of %s in %f microseconds.\n " , totalhits, input[1] , totaltime );
      totalhits=0; 
      close(spfd); //close the file
   }
  /* This command is going to replace the words based on the requirements
    * the exception handling is the same as before in the search
    * in this case we will make that will replace the word long
    * enough so that it matches the word we are replacing
    *thread function is the meat of the operation */
   else if( strcmp(input[0], "REPLACE") == 0) //time to replace!
   {
     if( atoi(input[3]) > 100 || atoi(input[3]) <= 0)
     {
       printf("Please enter a valid number of workers or refer to 'help'. \n");
       continue;
     }
    
     if( strstr(" ", input[1]) != NULL || strstr(" ", input[2]) != NULL )
     {
       printf("Please enter a valid word or refer to 'help'. \n");
       continue;
     }
        
     workers= atoi(input[3]);
     querysize = strlen(input[1]);
     strcpy(wordQ, input[1]);
     strcpy(wordR, input[2]);
     wdiff = strcmp( wordQ, wordR );
    
     if( wdiff != 0) //find out of the word is different
     {
      wdiff = strlen(wordQ) - strlen(wordR); //find the difference in word length
      for( i = 0; i< wdiff ; i++) strcat(wordR," "); //padd with spaces
     }
     else
     {
       printf("You entered the same word, try again.\n"); //check if its the same word
       continue;
     }

     if(( spfd = open("shakespeare.txt",O_RDWR)) < 0 ) //open in RW mode
     {
       perror("open");
       exit(1);
     }

     if((fstat(spfd, &file_stats)) < 0)
     {
       perror("stat");
       close(spfd);
       exit(1);
     }
       
     totalbytes = file_stats.st_size;
         
     searchp = mmap((caddr_t)0, totalbytes, PROT_READ | PROT_WRITE, MAP_SHARED, spfd, 0); //MUST BE IN READ WRITE MODE
     if( searchp == (caddr_t)(-1) ) //did our mapping work properly?
     {
       perror("mmap");
       exit(1);
     }

     //same shenanigans below
     for( currentworker=0; currentworker < workers; currentworker++)
     {
       rets[currentworker] = pthread_create( &threads[currentworker], NULL, &threadReplace, currentworker);
       if ( rets[currentworker] != 0) perror("pthread_create");
       
     }
      
     for( currentworker = 0; currentworker < workers; currentworker++ )
     {
       pthread_join(threads[currentworker], NULL);
     }

     printf( "Replaced %d instances of %s with %s.\n" , totalhits, input[1] , input[2] );
     totalhits=0; 
     close(spfd);


   }
   else if( strcmp(input[0], "HELP") == 0) //straight forward command instructions 
   {
     printf("\nShakespeare Word Search Service Command Help\n");
     printf("______________________________________________ \n");
     printf("\n");
     printf("help   - displays this message\n");
     printf("quit   - exits\n");
     printf("search [word] [workers] - searches the works of Shakespeare for [word] using\n");
     printf("                          [workers].  [workers] can be from 1 to 100.\n");
     printf("replace [word1] [word2] [workers] - search the works of Shakespeare for\n");
     printf("                          [word 1] using [workers] and replaces each\n");
     printf("                          instance with [word2]. [workers] can be from\n");
     printf("                          1 to 100\n"); 



 }
   else if( strcmp(input[0],"RESET") == 0)
   {
    //lets fork a process, copy the back as itself with a different name using execl
    resetpid = fork();
    
    if( resetpid < 0) 
    {
      perror("fork");
      continue;
    }
    if( resetpid == 0)
    {
     execl("/usr/bin/cp", "cp", "shakespeare_backup.txt", "shakespeare.txt",NULL);
     exit(0);
    }
   }
   else if( strcmp(input[0], "QUIT") == 0) //easy quit function to leave main
   {
    exit(0);
   }
   else //failed command
   {
      printf("Invalid command. Try again.\n");
   }
 }



 return 0 ;
}



/* The threadSearch function is for each individual thread to execute.
 * The current worker integer parameter is used to calculate the appropriate bytes for each thread.
 * There is a lot of repeated variables in to improve performance
*/

void* threadSearch( int currentworker )
{
  int beginningbyte, endingbyte, tempsize;
  char *tempmap;
  char tempq[31];
    
/* We are locking this part of the code to copy some global variables for individual use
 * The reason we copy the variables is to avoid using a mutex in a huge iteration
 * The thing that is called the most is memcmp and the for loop increments
 * by having a person variable we avoid a mutex, and therefore increase performance
 */

  pthread_mutex_lock(&mutexa);
    
    tempmap= searchp;
    strcpy(tempq,wordQ);
    tempsize = querysize;
    beginningbyte = ( currentworker / workers ) * totalbytes; //paritioning the worker space
    endingbyte = ( ( currentworker + 1 ) / workers ) * totalbytes;
   
  pthread_mutex_unlock(&mutexa);
    
    
  for( beginningbyte; beginningbyte < endingbyte ; beginningbyte++ ) //notice how for loop isnt mutexed
  {
        
    if( memcmp( tempmap + beginningbyte , tempq , tempsize ) == 0 ) //each call to memcmp can be done in parallel
    {
      pthread_mutex_lock(&mutexa); //this part will be locked because total hits is the gold of the search and is shared
        totalhits++;
      pthread_mutex_unlock(&mutexa); //unlock appropriately plzzzz
        
    }
        
  }
    
}

/* threadReplace function is for execution of threads to replace words
 * the parameter used is the current worker, we want to keep track of which thread we are in
 * so we can correctly calculate and partition the work
 * this function doesn't return anything
 */

void* threadReplace( int currentworker )
{
  int i, beginningbyte, endingbyte, tempsize;
  char* tempmap;
  char tempq[31],tempR[31];
    
/* We are locking this part of the code to copy some global variables for individual use
 * The reason we copy the variables is to avoid using a mutex in a huge iteration
 * The thing that is called the most is memcmp and the for loop increments
 * by having a person variable we avoid a mutex, and therefore increase performance
 */

  pthread_mutex_lock(&mutexa);
      
    strcpy(tempq,wordQ);
    strcpy(tempR,wordR);
    tempsize = querysize;
    beginningbyte = ( currentworker / workers ) * totalbytes;
    endingbyte = ( ( currentworker + 1 ) / workers ) * totalbytes;
    tempmap = searchp;

  pthread_mutex_unlock(&mutexa);
    
  for( beginningbyte; beginningbyte < endingbyte ; beginningbyte++ )
  {
     
    if( memcmp( tempmap + beginningbyte, tempq , tempsize ) == 0 )
    {
     
      pthread_mutex_lock(&mutexa);
        memcpy( tempmap + beginningbyte,tempR, tempsize); //lets replace the word we have created in main 
        totalhits++;// total hits is to show the user how many we replaced
      pthread_mutex_unlock(&mutexa); // this area is locked because we are accessing shared memory and writing to it
    
    }
   
     
  } 
 
}
