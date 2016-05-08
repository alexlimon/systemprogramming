/*
 * Name: Alex Limon
 * ID #: 1000818599
 * Programming Assignment 4
 * Description: Dropbox like filesystem.
 */

/* Include the correct libraries */
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
#include <time.h>

/* Defining some constants for our file system */
#define MAXARG 3
#define NUM_BLOCKS 1280
#define BLOCKSIZE  4096
#define NUM_FILES  128
#define FILE_DBS   32
#define MAXFILESIZE 131072


/*Function definitions for useful getters */
int bytesLeft();
int nextFreeIndex();
int getFileIndex();
int nextFreeBlock();


/* Keeps track of all the free blocks by '0' or '1' */
int freeblocks[1280];

/* Actual data of our file system */
unsigned char datablocks[NUM_BLOCKS][BLOCKSIZE];

/* Metadata for every file, and keeps track of its blocks */
struct FileMD
{
  char name[256];
  char date[30];
  int filesize;
  int fileblocks[FILE_DBS];
  int valid;
}directory[NUM_FILES];



int main( void ) 
{
 
 
  
  char *cmd;//this is the command that will be taken in from the stream
  char **inputS;// array of strings is for finding out the different commands
  char *args;
  int inputIndex = 0,i,j;
  int longfilenameflag = 0;
  int status, tempfilesize, offset, blockindex,inputfilebytes,writebytes;
  struct stat fileinfo; 
  char tempfilename[256];
  char currentdirectory[20];
  FILE *infp, *outfp;
 
/* Assigning all blocks as free when we start */
  for( i = 0; i < NUM_BLOCKS ; i++) freeblocks[i] = 0;

/* Assigning all files as unvalid so they can be written to */  
  for( i = 0; i < NUM_FILES; i++) directory[i].valid = 0;
  
  blockindex = 0;

  while(1) // doesn't stop until quit  or exit
  {
   
    cmd = (char*)malloc(sizeof(char)*50); // allocating the memory now instead of outside so it clears everytime
    inputS = (char**)malloc( sizeof(char*) * MAXARG );
    
    for( inputIndex = 0; inputIndex < MAXARG; inputIndex++ ) inputS[inputIndex] = (char*)malloc(sizeof(char)*256); 

    printf("mfs> "); // basic shell prompt
    fgets(cmd,400,stdin); // gets input from user 
    
    inputIndex = 0; // making sure i doesn't have a garbage or previous value
   
    while( (args = strsep(&cmd," \n\r")) != NULL ) // we will end once no input exists anymore by having a pointer
    {
      if(strstr(" ",args) == NULL) // sometimes strsep puts empty strings in our array, lets avoid this
      {
        if ( strlen(args) > 255 ) longfilenameflag = 1;
        inputS[inputIndex]= strndup(args,255); // putting the parsed content nicely in the input array
        inputIndex++; 
      }
  
      if( inputIndex == MAXARG ) break; // if theres too many don't even try to parse all the arguments
         
    }
    
    if( inputIndex == 0 ) continue; //there was nothing in there, prompt mfs again
  

/*Let's check for the put command */
    if( strcmp( inputS[0], "put" ) == 0  )
    {
      //copying the file name into a variable
      strncpy( tempfilename, inputS[1], 256);

      //we need some information about the file we are putting
      //so lets call the stat structure that was intialized
      status = stat( tempfilename, &fileinfo );
     
      // making sure that our file is valid before we start doing work 
      if( status != -1 )
      { 
        // this is an offset used to traverse at the size of each block
        offset = 0;
        //open the file
        infp = fopen( tempfilename, "r" );
        //find out it's size
        tempfilesize= (int) fileinfo.st_size ;
        
/*Checking for whether the file name is too long, if
 * there isn't enough diskspace, or if the file isn't in the directory
*/     
        if( bytesLeft() < tempfilesize || nextFreeIndex() == -1 || tempfilesize > MAXFILESIZE  )
        { 
          printf("put error: Not enough disk space\n");
          continue;
        }
        
        // a flag was set earlier to check how long the input is
        if(longfilenameflag)
        {
          printf("put error: File name too long.\n");
          longfilenameflag = 0;
          continue;
        }
       
 /* We are good to go!*/

      
       
        /*setting up the new file*/
        int currentfileindex = nextFreeIndex();
        
        /*We will begin to grab the date, unfortunately theres some messes to fix
         * the time outputs the day of the week 
         * the time doesnt have a null terminating character
         * the time has the year
         */
        time_t t;
        time(&t);
        char rawtime[30];
        char *begin;
        
        strncpy( rawtime, ctime(&t), 30);
        begin = &rawtime[4];
        strncpy( directory[currentfileindex].date, begin, 26);
        
        if(directory[currentfileindex].date[strlen(directory[currentfileindex].date)-1] == '\n')
        {
         directory[currentfileindex].date[strlen(directory[currentfileindex].date)-1] = '\0';
        }


        /* Lets now fill in the validity of the data
         * the name of the file
         * the file size
         */
        strncpy( directory[currentfileindex].name, tempfilename, 255 );
        directory[currentfileindex].filesize = tempfilesize;
        directory[currentfileindex].valid = 1;
        for( i = 0; i < FILE_DBS; i++ ) directory[currentfileindex].fileblocks[i] = -1;
        
        j=0; //this will be used to iterate through fileblocks inside the struct
        
        // while the bytes we are putting are greater than 0
        while( tempfilesize > 0)
        { 
          //lets find the next free block and assign it accordingly
          blockindex = nextFreeBlock();
          // seek the file pointer and update with the offset 
          fseek( infp, offset, SEEK_SET );
          // read from the file pointer only a blocksize at a time
          inputfilebytes = fread( datablocks[blockindex], BLOCKSIZE, 1, infp );
          // making sure that nothing weird is happening
          if( inputfilebytes == 0 && !feof( infp ) )
          {
            printf("An error has occured with the file specified\n");
            break;
          }
          
          // store the index file blocks in the structure and make sure it is no longer free
          directory[currentfileindex].fileblocks[j] = blockindex; 
          freeblocks[blockindex] = 1;
          clearerr( infp );
          
          //increment j for the fileblocks in the metadata   
          j++;
          // the current file we are putting is decreased after we read it
          tempfilesize -= BLOCKSIZE;
          offset += BLOCKSIZE;
          
        }
        fclose( infp );  
      }
      else //at this point, no such file exists give an error
      {
        printf("put error: No such file found\n");
        continue;
      }

    }//end of put command
    else if( strcmp("df", inputS[0]) == 0 )
    { //calculate free bytes through a function
      printf("%d bytes free\n", bytesLeft());
    } //end of df command
    else if( strcmp("list", inputS[0]) == 0)
    {
      // go through all the directory and show only the valid ones
      for( i = 0; i < NUM_FILES; i++)
      {
        if( directory[i].valid == 1)
        {
          //print out the correct values
          printf("%d %s %s \n",directory[i].filesize, directory[i].date, directory[i].name);
        }   

      } 
    } //end of list command
   else if( strcmp("del",inputS[0]) == 0)
   { 
     //get the file name and take out the corresponding data 
     strncpy( tempfilename, inputS[1], 256);
     int delfileindex = getFileIndex( tempfilename );
     if( delfileindex == -1 )
     { // we couldnt find it in the directory
       printf("del error: File not found\n");
     }
     else
     {
       // lets start by unvalidating the file
       directory[delfileindex].valid = 0;
       
       for (i = 0 ; i < FILE_DBS; i++)
       { // go through the file blocks and make sure they are denoted as free 
         if( directory[delfileindex].fileblocks[i] != -1 )
         {
           freeblocks[directory[delfileindex].fileblocks[i]] = 0;
           directory[delfileindex].fileblocks[i] = -1;
         }
       }
     }

   }
   else if(strcmp("get", inputS[0]) == 0)
   {
     //check to see if we have a second argument
     if( strlen(inputS[2]) > 1)
     {
       // grab the second argument as the new name
       strncpy( tempfilename, inputS[2], 256 );
     }
     else
     {
       //grab the first argument for the same name
       strncpy( tempfilename, inputS[1], 256 );
     }
     // get the file index to see where we should write
     int getfileindex = getFileIndex( inputS[1] );
     if( getfileindex == -1)
     { 
       printf("get error: File not found\n");
     }
     else
     {
       // open a new file to write the bytes in the struct
       outfp = fopen( tempfilename, "w");
       
       if( outfp == NULL )
       {
         printf("get error: File not found\n");
         break;
       }
      
       //get the filesize of it to start decrementing
       tempfilesize = directory[getfileindex].filesize;
       offset = 0;
       j = 0;
       
       // while we are not at the end of the bytes or we have not gotten over the direct blocks
       while( tempfilesize > 0 || j > 32 )
       {
         //get the index of the next block 
         blockindex = directory[getfileindex].fileblocks[j];
         if( tempfilesize < BLOCKSIZE )
         { // when we are at the last block take in whats left
           writebytes = tempfilesize;
         }
         else
         {
           // just a full block needed to be written
           writebytes = BLOCKSIZE;
         }
         // write the block necessary with either a block size or less than a blocksize
         fwrite( datablocks[blockindex], writebytes, 1, outfp );
         // decrement how much we have written
         tempfilesize -= BLOCKSIZE;
         // increase for the next chunk
         offset += BLOCKSIZE;
         // increase the j for the fileblocks
         j++;
         
         fseek(outfp, offset, SEEK_SET);
      }  
      
      fclose(outfp);
    } 
  }// end of get command
  else if( strcmp("quit",inputS[0]) == 0) exit(0);
  // check to see if we need to quit
  else{}

  } //end of main while loop


  return 0 ;
}

/* nextFreeIndex will get the next free index in the directory to allocate the file to  and return that number*/
int nextFreeIndex()
{
  int i = 0;

  for(i = 0; i < NUM_FILES ; i++) 
  {
    if( !directory[i].valid ) break;
  }
  if (i == NUM_FILES ) i = -1;
  return i;
}

/* next free block finds the next free block in the datablocks and returns that index
 * it returns -1 if the data is full
 */

int nextFreeBlock()
{
  int i = 0;

  for(i = 0; i < NUM_BLOCKS ; i++) 
  {
    if( !freeblocks[i] ) break;
  }
  if (i == NUM_BLOCKS ) i = -1;
  return i;
}

/*  bytesLeft calculates the amount of free space that is left in the filesystem and returns that value as an integer */

int bytesLeft()
{
  int i;
  int count = 0;
  
  for(i = 0; i <NUM_BLOCKS ; i++)
  {
    if( freeblocks[i] == 0) count++;
  }

  return count*BLOCKSIZE;
}


/* getFileIndex takes in a file name, and returns the index of that file in the directory 
 * if that file doesn't exist it will return -1
 */
int getFileIndex( char filename[] )
{
  int i;
   
  for( i = 0; i < NUM_FILES; i++ )
  {
    if( directory[i].valid )
    {
      if( strcmp( directory[i].name, filename ) == 0 )
      {
        return i;
      }
    }
  }
  return -1;
}




