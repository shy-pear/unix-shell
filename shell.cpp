#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

using namespace std;

#define MAX_LINE 80 /* The maximum length command */

int main(void){

  /*command to store user input*/
 char *command = new char[MAX_LINE]; 

  /* command line arguments */
  char *args[MAX_LINE/2 + 1] = {nullptr};

  /* second set of command line arguments in case of pipe */
  char *pipeArgs[MAX_LINE/2 + 1] = {nullptr}; 

  /*string array to hold history args*/
  char *history[MAX_LINE/2 + 1] = {nullptr}; 

  /*delimiter to separate args in command*/
  const char* delimiter = " ";

  /*boolean variables to decide if command has pipe and if parent needs to wait*/
  bool hasPipe = false;
  bool doesntNeedToWait = false;
 


  /**
   * While the user does not enter command "exit", should_run should be 1"
  */
  int should_run = 1; /* flag to determine when to exit program */

  while (should_run == 1) {
    printf("osh>");
    fflush(stdout);

    //------------read user input for command-----------------------

    //get user input and store into command
    fgets(command, MAX_LINE, stdin);

    //-----------------------handle command-------------------------

    //parse command
    char* token = strtok(command, delimiter);

    //if command is null, continue
    if (*token == '\n'){
      continue;
    }

    //if command is not null, remove new line char
    int k = 0;
    while (k < MAX_LINE){
      if (*(command + k) == '\n'){
        *(command + k) = 0;
        break;
      }
      k++;
    }

    //if first arg is "exit", exit the program
    if (strcmp(token, "exit") == 0){
      should_run = 0;
      continue;
    }

    //if first arg is "!!", return the last command stored in history
    if (strcmp(token, "!!") == 0){

      //if no commands in history, print out message to user's screen
      if (history[0] == 0){
        cout << "No commands in history." << endl;
        continue;
      }

      /*if command stored in history, it should be echoed to user's screen
      and stored as new command*/
      int i = 0;

      /*historyCommand stores the command from history*/
      char *historyCommand = new char[MAX_LINE];
      historyCommand[0] = '\0';

      while (history[i] != 0){
        strcat(historyCommand, history[i]);
        strcat(historyCommand, " ");
        i++;
      }

      cout << historyCommand << endl;

      /*get tokens from historyCommand*/
      token = strtok(historyCommand, delimiter);

    }

    /*clear current history array to store new args*/
    int m = 0;
    while (history[m] != 0){
      history[m] = 0;
      m++;
    }

    /* Store the command in args and history.
    -- If there is a | symbol, set hasPipe to true and 
    store rest of args in pipargs 
    -- If there is a & symbol, set doesntNeedToWait to true
    */
    int i = 0;
    int j = 0;
    while (token != NULL){
      //if token == |
      if (strcmp(token, "|") == 0){
        hasPipe = true;
        i = 0;

        //add token to history
        history[j] = new char[strlen(token) + 1];
        strcpy(history[j], token);
        j++;

        token = strtok(NULL, delimiter);
        continue;
      }

      //if token == &
      if (strcmp(token, "&") == 0){
        doesntNeedToWait = true;

        //add token to history
        history[j] = new char[strlen(token) + 1];
        strcpy(history[j], token);
        j++;

        token = strtok(NULL, delimiter);
        continue;

      }

      //if command has a pipe
      if (hasPipe){
        pipeArgs[i] = new char[strlen(token) + 1];
        strcpy(pipeArgs[i], token);
        i++;
      } 
      //if command doesn't have a pipe
      else {
        args[i] = new char[strlen(token) + 1];
        strcpy(args[i], token);
        i++;
      }

      //add token to history
      history[j] = new char[strlen(token) + 1];
      strcpy(history[j], token);
      j++;

      //get next token
      token = strtok(NULL, delimiter);
    }

  
    //------------fork a child process using fork()-----------------
    pid_t pid = fork();
    //check for error
    if (pid < 0){
      cerr << "Error with fork." << endl;
      exit(1);
    }

    //child process
    else if (pid == 0){

      /*If there's a pipe in the command*/
      if (hasPipe){
        /*create pipe for child1 (original child) and child2 (child of child) 
        to communicate*/
        int fdArray[2];
  
        int pipeNum = pipe(fdArray);
        //check for error
        if (pipeNum < 0){
          cerr << "Error with pipe." << endl;
          exit(1);
        }
        //fork the child
        pid_t pid2 = fork();
        //check for error
        if (pid2 < 0){
          cerr << "Error with second fork." << endl;
          exit(1);
        }
        /*Child2 code
        -- close the read pipe fd
        -- redirect output stream to write pipe fd
        -- call execvp with args
        */
        else if (pid2 == 0){
          //-----child2 code------
          cout << "inside child2" << endl;
          close(fdArray[0]);
          dup2(fdArray[1], STDOUT_FILENO);

          execvp(args[0], args);
          exit(0);

        }
         /*Child1 code
        -- close the write pipe fd
        -- redirect input stream to read pipe fd
        -- call execvp with pipeArgs
        */
        else if (pid2 > 0) {
          //----child1 code-------
          wait(NULL);
          close(fdArray[1]);
          dup2(fdArray[0], STDIN_FILENO);

          execvp(pipeArgs[0], pipeArgs);
          cout << "inside child1" << endl;
          exit(0);

        }


      } 
      /*If no pipe, check for file redirects*/
      else {
        //get the redirect index
        int i = 0;
        int redirectInIndex = -1;
        int redirectOutIndex = -1;
        while (args[i] != 0){
          if (strcmp(args[i], "<") == 0){
            redirectInIndex = i;
            break;
          } else if (strcmp(args[i], ">") == 0){
            redirectOutIndex = i;
            break;
          }
          i++;
        }
        /*If input file redirect*/
        if (redirectInIndex != -1){
          int fd = open(args[redirectInIndex+1], O_RDONLY);
          /*If file doesn't exist, print error message to console*/
          if (fd == -1){
            cerr << "No such file exists." << endl;
            exit(1);
          }
          /*redirect input stream to fd and close fd*/
          dup2(fd, STDIN_FILENO);
          close(fd);
          /*change args to null*/
          args[redirectInIndex] = NULL;
          args[redirectInIndex + 1] = NULL;
          
        }
        /*If output file redirect*/
        else if (redirectOutIndex != -1){
          int fd = open(args[redirectOutIndex+1], O_WRONLY | O_CREAT, 0666);
          /*redirect output stream to fd and close fd*/
          dup2(fd, STDOUT_FILENO);
          close(fd);
          /*change args to null*/
          args[redirectOutIndex] = NULL;
          args[redirectOutIndex + 1] = NULL; 
        }

      }

      /*call execvp with args*/
      execvp(args[0], args);
      exit(0);


    }

    //parent process
    else if (pid > 0) {
      
      /*If parent needs to wait before executing code, call wait here*/
      if (!doesntNeedToWait){
        wait(NULL);
      }
      //clear the args[] array
      int i = 0;
      while (args[i] != 0){
        delete[] args[i];
        args[i] = 0;
        i++;
      }
      //clear the pipeArgs[] array
      i = 0;
      while (pipeArgs[i] != 0){
        delete[] pipeArgs[i];
        pipeArgs[i] = 0;
        i++;
      }

      //reset hasPipe and doesntNeedToWair
      hasPipe = false;
      doesntNeedToWait = false;

      /*Call wait at the end in case the child process is still running and
      & was included, so parent didn't wait at the beginning. Ensures no
      zombie children and errors*/
      wait(NULL);

    }

  }
  delete[] command;

  return 0;


}