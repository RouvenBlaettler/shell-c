#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  char input[256];

  char commands[3][256] = {"exit", "echo", "type"};
  int size = sizeof(commands) / sizeof(commands[0]);
  // TODO: Uncomment the code below to pass the first stage

  while(1){
    printf("$ ");
    fgets(input,sizeof(input), stdin);
    input[strlen(input)-1] = '\0';

    if(strcmp(input, "exit") == 0){
      break;
    }
    else if(strncmp(input, "echo ", 5) == 0){
      printf("%s\n", input + 5);

    }
    else if(strncmp(input, "type ", 5) == 0){
      int tmp = 0;
      for(int i = 0; i<size; i++){
        if(strcmp(input + 5, commands[i]) == 0){
          printf("%s is a shell builtin\n", input + 5);
          tmp++;
          }
        }
      if(tmp == 0){
      printf("%s: command not found\n", input + 5);
      }
    }
    else{
      printf("%s: command not found\n", input);
      }
    }

  return 0;
}
