#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <linux/limits.h>
#include <pwd.h>

const int MAX_COMMAND_LEN = 100;
const int MAX_COMMANDS = 100;

typedef struct {
  char command[100];
  char status[10]; //RUNNING or DONE
  pid_t pid;
  bool isBackground;
} DataOfCommand;

int main() {

  char* command = malloc(MAX_COMMAND_LEN);
  char* commandCopy = malloc(MAX_COMMAND_LEN);
  char* echoCommand = malloc(MAX_COMMAND_LEN);

  char *token;
  char *commandName = malloc(MAX_COMMAND_LEN * sizeof(commandName));
  char *commandArgs[MAX_COMMAND_LEN]; //used mainly by execv
  int lenOfCommandsArgs;

  DataOfCommand *history[MAX_COMMANDS]; //Data of commands
  int historyCounter = 0;

  bool isEcho;

  unsigned long i; //index

  //initializing relevant directories
  char currentDir[PATH_MAX];
  getcwd(currentDir, sizeof(currentDir));
  char previousDir[PATH_MAX] = {};
  char *homeDir;
  if ((homeDir = getenv("HOME")) == NULL) {
    homeDir = getpwuid(getuid())->pw_dir;
  }

  int stat;
  pid_t pid;

  //the program ends if the user writes exit
  while (1) {
    lenOfCommandsArgs = 0;
    isEcho = false;

    usleep(4000); //sleep for 0.004 seconds (relevant for background processes with prints)
    //prompt:
    printf("> ");
    fgets(command, MAX_COMMAND_LEN, stdin);


    //removing \n
    if (command[strlen(command) - 1] == '\n') {
      command[strlen(command) - 1] = 0;
    }

    //dealing with echo command (strtok isn't helpful enough):
    if (strlen(command) >= 5) {
      char* check = command;
      i = 0;
      while(check[i] == ' ')
      {
        i++;
      }
      if(strncmp(&check[i], "echo ", 5) == 0)
      {
        isEcho = true;
        commandArgs[lenOfCommandsArgs] = malloc(strlen("echo") + 1);
        strcpy(commandArgs[lenOfCommandsArgs], "echo");
        bool inBrackets = false, inArg = false;
        int sizeOfArg = 0;
        char* arg = malloc(MAX_COMMAND_LEN);
        i += 5;
        while (command[i] != 0 || inArg == true) {

          if(inArg == true)
          {
            if(command[i] == 0)
            {
              commandArgs[lenOfCommandsArgs] = malloc(strlen(arg) + 1);
              strcpy(commandArgs[lenOfCommandsArgs], arg);
              free(arg);
              inArg = false;
              continue;
            }
            else if(command[i] == ' ')
            {
              if(inBrackets == true)
              {
                arg[sizeOfArg] = ' ';
                sizeOfArg++;
              }
              else
              {
                arg[sizeOfArg] = 0;
                commandArgs[lenOfCommandsArgs] = malloc(strlen(arg) + 1);
                strcpy(commandArgs[lenOfCommandsArgs], arg);
                free(arg);
                inArg = false;
              }
              i++;
            }
            else if(command[i] == '"' && command[i-1] != '\\')
            {
              arg[sizeOfArg] = 0;
              commandArgs[lenOfCommandsArgs] = malloc(strlen(arg) + 1);
              strcpy(commandArgs[lenOfCommandsArgs], arg);
              free(arg);
              inBrackets = false;
              inArg = false;
              i++;
            }
            else if(command[i] == '\\' && command[i+1] == '"')
            {
              arg[sizeOfArg] = '\"';
              sizeOfArg++;
              i += 2;
            }
            else
            {
              arg[sizeOfArg] = command[i];
              sizeOfArg++;
              i++;
            }
          }
          else
          {
            if(command[i] == ' ')
            {
              i++;
            }
            else if(command[i] == '"' && command[i-1] != '\\')
            {
              lenOfCommandsArgs++;
              arg = malloc(MAX_COMMAND_LEN);
              sizeOfArg = 0;
              inBrackets = true;
              inArg = true;
              i++;
            }
            else if(command[i] == '\\' && command[i+1] == '"')
            {
              lenOfCommandsArgs++;
              arg = malloc(MAX_COMMAND_LEN);
              sizeOfArg = 0;
              arg[sizeOfArg] = '\"';
              inArg = true;
              sizeOfArg++;
              i += 2;
            }
            else
            {
              lenOfCommandsArgs++;
              arg = malloc(MAX_COMMAND_LEN);
              sizeOfArg = 0;
              arg[sizeOfArg] = command[i];
              sizeOfArg++;
              inArg = true;
              i++;
            }
          }
        }

        if(commandArgs[lenOfCommandsArgs] == NULL)
        {
          lenOfCommandsArgs--;
        }
      }
    }

    //parsing other commands
    if(!isEcho)
    {
      strcpy(commandCopy, command);
      //parsing:
      for (token = strtok(command, " "); token; token = strtok(NULL, " ")) {
        commandArgs[lenOfCommandsArgs] = malloc(strlen(token) + 1);
        strcpy(commandArgs[lenOfCommandsArgs], token);
        lenOfCommandsArgs++;
      }
      lenOfCommandsArgs--;
      strcpy(command, commandCopy);
    }

    //dealing with exit command
    if (strcmp(commandArgs[0], "exit") == 0 && lenOfCommandsArgs == 0) {
      printf("%d\n", getpid());
      free(command);
      free(commandCopy);
      free(echoCommand);
      free(commandName);
      for (i = 0; i <= lenOfCommandsArgs; i++) {
        if(commandArgs[i] != NULL)
        {
          free(commandArgs[i]);
        }
      }

      for (i = 0; i < historyCounter; i++) {
        free(history[i]);
      }

      exit(EXIT_SUCCESS);
    }

    if (strcmp(commandArgs[0], "cd") == 0) {
      char tmp[PATH_MAX] = {}; //used if needed
      printf("%d\n", getpid());
      if (lenOfCommandsArgs > 1)
      {
        fprintf(stderr, "Error: Too many arguments\n");
      }
      else if (lenOfCommandsArgs == 0 || strcmp(commandArgs[1], "~") == 0) {
        if (chdir(homeDir) != 0) {
          fprintf(stderr, "‫‪Error‬‬ ‫‪in‬‬ ‫‪system‬‬ ‫‪call\n");
        }
        else {
          strcpy(previousDir, currentDir);
          strcpy(currentDir, homeDir);
        }
      }
      else {
        if (strcmp((const char *) commandArgs[1], "-") == 0) {
          //cd -
          if (strcmp(previousDir, "") != 0) {
            if (chdir(previousDir) != 0) {
              fprintf(stderr, "‫‪Error‬‬ ‫‪in‬‬ ‫‪system‬‬ ‫‪call\n");
            } else {
              strcpy(tmp, previousDir);
              strcpy(previousDir, currentDir);
              strcpy(currentDir, tmp);
            }
          }
        }
        else if (strncmp((const char *) commandArgs[1], "~/", 2) == 0) {
          //cd ~/path
          strcpy(tmp, homeDir);
          strcat(tmp, "/");
          strcat(tmp, &commandArgs[1][2]);
          if (chdir(tmp) != 0) {
            fprintf(stderr, "‫‪Error‬‬ ‫‪in‬‬ ‫‪system‬‬ ‫‪call\n");
          } else {
            strcpy(previousDir, currentDir);
            getcwd(currentDir, sizeof(currentDir));
          }

        }
        else if (chdir(commandArgs[1]) != 0) {
          fprintf(stderr, "‫‪Error‬‬ ‫‪in‬‬ ‫‪system‬‬ ‫‪call\n");
        }
        else {
          //other cd's
          strcpy(previousDir, currentDir);
          getcwd(currentDir, sizeof(currentDir));
        }
      }

      //adding cd command to history

      history[historyCounter] = malloc(sizeof(DataOfCommand));
      history[historyCounter]->isBackground = false;
      history[historyCounter]->pid = getpid();
      strcpy(history[historyCounter]->command, command);
      strcpy(history[historyCounter]->status, "DONE");
      historyCounter++;
    }
    else {
      if (strcmp(commandArgs[lenOfCommandsArgs], "&") == 0) {
        //Background
        commandArgs[lenOfCommandsArgs] = NULL;
        sprintf(commandName, "/bin/%s", commandArgs[0]);

        int retCodeBackground;
        pid = fork();

        if (pid < 0) {
          fprintf(stderr, "‫‪Error‬‬ ‫‪in‬‬ ‫‪system‬‬ ‫‪call\n");
        }
        else if (pid == 0) {
          //child
          retCodeBackground = execv(commandName, commandArgs);
          if (retCodeBackground == -1) {
            fprintf(stderr, "‫‪Error‬‬ ‫‪in‬‬ ‫‪system‬‬ ‫‪call\n");
          }
          exit(0);
        }
        else {
          //parent
          printf("%d\n", pid); //printing pid of child

          //adding background command to history
          history[historyCounter] = malloc(sizeof(DataOfCommand));
          history[historyCounter]->isBackground = true;
          history[historyCounter]->pid = pid;
          strcpy(history[historyCounter]->command, command);
          strcpy(history[historyCounter]->status, "RUNNING");
          historyCounter++;
        }
      } //Background
      else {
        //Foreground
        commandArgs[lenOfCommandsArgs + 1] = NULL;
        sprintf(commandName, "/bin/%s", commandArgs[0]);

        //refreshing status of background commands if needed
        if (strcmp(command, "jobs") == 0 || strcmp(command, "history") == 0) {
          for (i = 0; i < historyCounter; i++) {
            if (history[i]->isBackground == true && strcmp(history[i]->status, "RUNNING") == 0) {
              if (waitpid(history[i]->pid, &stat, WNOHANG) != 0) {
                strcpy(history[i]->status, "DONE");
              }
            }
          }
        }

        int waitedForeground, retCodeForeground;
        pid = fork();

        if (pid < 0) {
          fprintf(stderr, "‫‪Error‬‬ ‫‪in‬‬ ‫‪system‬‬ ‫‪call\n");
        }
        else if (pid == 0) {
          //child
          if (strcmp(command, "jobs") == 0) {
            //jobs command
            for (i = 0; i < historyCounter; i++) {
              if (history[i]->isBackground == true && strcmp(history[i]->status, "RUNNING") == 0) {
                char * commandWithoutAmpersand = malloc(MAX_COMMAND_LEN);
                strcpy(commandWithoutAmpersand, history[i]->command);
                commandWithoutAmpersand[strlen(commandWithoutAmpersand) - 2] = 0; //removing &
                printf("%d %s\n", history[i]->pid, commandWithoutAmpersand);
                free(commandWithoutAmpersand);
              }
            }
            exit(0);
          }
          else if (strcmp(command, "history") == 0) {
            //history command
            for (i = 0; i < historyCounter; i++) {
              printf("%d %s %s\n", history[i]->pid, history[i]->command, history[i]->status);
            }
            printf("%d %s %s\n", getpid(), "history", "RUNNING");
            exit(0);
          }
          else {
            //exec commands
            retCodeForeground = execv(commandName, commandArgs);
            if (retCodeForeground == -1) {
              fprintf(stderr, "‫‪Error‬‬ ‫‪in‬‬ ‫‪system‬‬ ‫‪call\n");
            }
            exit(0);
          }
        }
        else {
          //parent
          printf("%d\n", pid);
          //waiting
          do {
            waitedForeground = waitpid(pid, &stat, WNOHANG); //waiting for the child process to finish
          } while (waitedForeground != pid);

          //adding foreground command to history
          history[historyCounter] = malloc(sizeof(DataOfCommand));
          history[historyCounter]->isBackground = false;
          history[historyCounter]->pid = pid;
          strcpy(history[historyCounter]->command, command);
          //Foreground processes will be DONE because they are certainly finished before history command
          strcpy(history[historyCounter]->status, "DONE");
          historyCounter++;
        }
      } //Foreground
    }

    //free commandArgs elements
    for (i = 0; i <= lenOfCommandsArgs; i++) {
      if(commandArgs[i] != NULL)
      {
        free(commandArgs[i]);
      }
    }
  }
}