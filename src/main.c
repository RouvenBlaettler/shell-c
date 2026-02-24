#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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
      const char *cmd = input + 5;
      int tmp = 0;
      for(int i = 0; i<size; i++){
        if(strcmp(cmd, commands[i]) == 0){
          printf("%s is a shell builtin\n", cmd);
          tmp++;
          }
        }
      if(tmp == 0){
        const char *path_env = getenv("PATH");
        int found = 0;

        if(path_env != NULL){
          char *paths = strdup(path_env);
          char *saveptr = NULL;

          for(char *dir = strtok_r(paths, ":", &saveptr); dir != NULL; dir = strtok_r(NULL, ":", &saveptr)){
            char full_path[512];
            if(snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd) >= (int)sizeof(full_path)){
              continue;
            }
            if(access(full_path, X_OK) == 0){
              printf("%s is %s\n", cmd, full_path);
              found = 1;
              break;
            }
          }

          free(paths);
        }

        if(found == 0){
          printf("%s: not found\n", cmd);
        }
      }
    }
    else{
      printf("%s: command not found\n", input);
      }
    }

  return 0;
}
