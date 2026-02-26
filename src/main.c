#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

char * check_if_executable(const char* cmd);
char** tokenize_input(char* input);
void free_tokens(char** tokens);


int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  char input[256];

  char commands[3][256] = {"exit", "echo", "type"};
  int size = sizeof(commands) / sizeof(commands[0]);

  while(1){
    printf("$ ");
    fgets(input,sizeof(input), stdin);
    input[strlen(input)-1] = '\0';

    char** tokens = tokenize_input(input);

    if(strcmp(tokens[0], "exit") == 0){
      break;
    }
    else if(strcmp(tokens[0], "echo") == 0){
      printf("%s\n", input + 5);

    }
    else if(strcmp(tokens[0], "type") == 0){
      const char *cmd = tokens[1];
      int tmp = 0;
      for(int i = 0; i<size; i++){
        if(strcmp(cmd, commands[i]) == 0){
          printf("%s is a shell builtin\n", cmd);
          tmp++;
        }
      }
      if(tmp == 0){
        char *result = check_if_executable(cmd);
        if(result){
          printf("%s is %s\n", cmd, result);
          free(result);
        }
        else{
          printf("%s: not found\n", cmd);
        }
      }
    }
    else if(check_if_executable(tokens[0])){
      system(input);
    }
    else{
      printf("%s: command not found\n", input);
    }
  }
        

  return 0;
}


char * check_if_executable(const char* cmd){
  const char *path_env = getenv("PATH");

  if(path_env != NULL){
    char *paths = strdup(path_env);
    char *saveptr = NULL;

    for(char *dir = strtok_r(paths, ":", &saveptr); dir != NULL; dir = strtok_r(NULL, ":", &saveptr)){
      char full_path[512];
      if(snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd) >= (int)sizeof(full_path)){
            continue;
          }
          if(access(full_path, X_OK) == 0){
            free(paths);
            return strdup(full_path);
          }
        }

        free(paths);
      }
  return NULL;
  }


  char** tokenize_input(char* input){
    char** tokens = malloc(64 * sizeof(char*));
    int count = 0;

    char* input_copy = strdup(input);
    char* saveptr = NULL;

    for(char* token = strtok_r(input_copy, " ", &saveptr); token != NULL; token = strtok_r(NULL, " ", &saveptr)){
      tokens[count++] = strdup(token);
    }
    tokens[count] = NULL;

    free(input_copy);
    return tokens;
  }

  void free_tokens(char** tokens) {
    for(int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}