/*
 * Name:Alex Limon	 
 * ID #:1000818599 
 * Programming Assignment 1
 * Description: Basic shell program than can execute various linux commands with or without parameters.
 */
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>


int main( void ) 
{
  char *cmd;//this is the command that will be taken in from the stream
  char **inputS;// array of strings is for finding out the different commands
  char directory[20]; // static directory for execl
  pid_t pid;// extra process in order to use execl without leaving
  char *args;
  int i,k,j=0,status;
  
  signal(SIGINT,SIG_IGN); // stopping Ctr-Z and Ctr-C from killing our shell
  signal(SIGTSTP,SIG_IGN);
 
  strcpy(directory,"/usr/bin/"); // putting in the location of all the commands so the cmd can be appended later
  
  while(1) // doesn't stop until quit  or exit
  {
   
    cmd= (char*)malloc(sizeof(char)*50); // allocating the memory now instead of outside so it clears everytime
    inputS=(char**)malloc(sizeof(char*)*5);
    for(i=0;i<6;i++) inputS[i]=(char*)malloc(sizeof(char)*20); 

    printf("msh> "); // basic shell prompt
    fgets(cmd,40,stdin); // gets input from user 
    
    i=0; // making sure i doesn't have a garbage or previous value
   
    while( (args = strsep(&cmd," \n\r")) != NULL ) // we will end once no input exists anymore by having a pointer
    {
      if(strstr(" ",args) == NULL) // sometimes strsep puts empty strings in our array, lets avoid this
      {
        inputS[i]= strndup(args,255); // putting the parsed content nicely in the input array
        i++; 
      }
  
      if(i == 4) break; // if theres too many don't even try to parse all the arguments
         
    }
  
    if(i==0) continue; // if it never had anything ask for input again by starting the while loop over
    
    for(i=i;i<5;i++) inputS[i]=NULL; // nulling out extra possible arguments so execl will be happy
    
    if(strcmp("quit",inputS[0])==0 || strcmp("exit",inputS[0])==0)  // checking for a possible quit or exit
    {
      break;
      exit(0); 
    }
    
    if(strcmp("cd",inputS[0]) == 0) // execl can't change directory so we have to do it ourselves
    {  
      if( chdir(inputS[1]) < 0 )  // change directory has negative value if it was unsuccessful
      {
        printf("%s does not exist.\n",inputS[1]);
        continue;
      }
      else continue; // once we have changed directory we want to know what else the user wants by forking
    
    }
    
    pid = fork(); // this process is for the exec calls that would kill the parent
    
    if( pid < 0) // just in case our process was never created. pid would be negative
    {
      printf("%s: Command could not be executed.\n",inputS[0]);
    }
    if( pid == 0 )
    {
      strcat(directory,inputS[0]); // concatinate the cmd to the cmd directory
      if( execl(directory,inputS[0],inputS[1],inputS[2],inputS[3],inputS[4]) < 0 ) // executing command in index 0, but before that include directory
      {
        printf("%s: Command not found.\n",inputS[0]); // -1 if command doesnt exist
        exit(0);
      }
       
    }
    else
    {
      waitpid(pid,&status,0); // have the parent wait for the exec call so it wont go out of order
    }  
   
   
 }

 return 0 ;
}
