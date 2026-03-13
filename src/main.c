#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>


char * check_if_executable(const char* cmd);
char** tokenize_input(char* input);
void free_tokens(char** tokens);
int token_count(char** tokens);


typedef struct {
  bool has_redirection;
  int target_fd;
  bool append;
  char *file;
  int saved_fd;
} Redirection;

typedef int (*CmdFn)(char **tokens, Redirection *r);

typedef struct {
  const char *name;
  CmdFn function;
} Builtin;

int exit_fn(char** tokens, Redirection *r);
int echo_fn(char** tokens, Redirection *r);
int type_fn(char** tokens, Redirection *r);
int cd_fn(char** tokens, Redirection *r);
int pwd_fn(char** tokens, Redirection *r);

int parse_redirection(char **tokens, Redirection *r, int token_amount);
int apply_redirection(Redirection *r, bool save_for_restore);
const Builtin *find_builtin(const char *cmd, const Builtin *builtins, int count);


Builtin builtins[] = {
    {"exit", exit_fn},
    {"echo", echo_fn},
    {"type", type_fn},
    {"pwd", pwd_fn},
    {"cd", cd_fn},
  };

int amount_builtins = sizeof(builtins) / sizeof(builtins[0]);



int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  char input[256];

  
  Redirection redir;
  Redirection  *r = &redir;

  while(1){
    printf("$ ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
      break;
    }
    input[strcspn(input, "\n")] = '\0';

    char** tokens = tokenize_input(input);
    int token_amount = token_count(tokens);
    if(tokens[0]==NULL){
      free_tokens(tokens);
      continue;
    }

    if (parse_redirection(tokens, r, token_amount) != 0) {
      free_tokens(tokens);
      continue;
    }
    bool save_for_restore;

    const Builtin *b = find_builtin(tokens[0], builtins, amount_builtins);
    if (b != NULL) {
      int rc = b->function(tokens, r);
      free_tokens(tokens);
      if (rc == 1) break;   // optional exit code convention
      continue;
    }
    

    char* path = check_if_executable(tokens[0]);
    if(path){
      save_for_restore = false;
      pid_t pid = fork();
      if(pid == 0){
      if (apply_redirection(r, save_for_restore) != 0) {
        exit(1);
      }
      execv(path, tokens);
      perror("exec");
      exit(1);
      }
      else{
        waitpid(pid, NULL, 0);
      }
      
      free(path);
      free_tokens(tokens);
      continue;
    }
    printf("%s: command not found\n", tokens[0]);
    
  free_tokens(tokens);
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

int token_count(char** tokens){
  int n = 0;
  while(tokens[n] != NULL){
    n++;
  }
  return n;
}

void free_tokens(char** tokens) {
  for(int i = 0; tokens[i] != NULL; i++) {
      free(tokens[i]);
  }
  free(tokens);
}

  
int parse_redirection(char **tokens, Redirection *r, int token_amount) {

  r->has_redirection = false;
  r->target_fd = 1;
  r->append = false;
  r->file = NULL;
  r->saved_fd = -1;

  for (int i = 0; i < token_amount; i++) {
    int target_fd = -1;
    bool append = false;

    if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "1>") == 0) {
      target_fd = 1; append = false;
    } else if (strcmp(tokens[i], "2>") == 0) {
      target_fd = 2; append = false;
    } else if (strcmp(tokens[i], ">>") == 0 || strcmp(tokens[i], "1>>") == 0) {
      target_fd = 1; append = true;
    } else if (strcmp(tokens[i], "2>>") == 0) {
      target_fd = 2; append = true;
    } else {
      continue;
    }

    if (i + 1 >= token_amount || tokens[i + 1] == NULL) {
      fprintf(stderr, "syntax error: expected file after redirection\n");
      return -1;
    }

    r->has_redirection = true;
    r->target_fd = target_fd;
    r->append = append;
    r->file = tokens[i + 1];
    tokens[i] = NULL;
    return 0;
  }

  return 0;
}

int apply_redirection(Redirection *r, bool save_for_restore){
  if (!r->has_redirection) {
    return 0;
  }

  int flags = O_WRONLY | O_CREAT | (r->append ? O_APPEND : O_TRUNC);
  int fd = open(r->file, flags, 0644);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  if (save_for_restore) {
    r->saved_fd = dup(r->target_fd);
    if (r->saved_fd == -1) {
      perror("dup");
      close(fd);
      return -1;
    }
  }

  if (dup2(fd, r->target_fd) == -1) {
    perror("dup2");
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

const Builtin *find_builtin(const char *cmd, const Builtin *builtins, int count) {
  for (int i = 0; i < count; i++) {
    if (strcmp(cmd, builtins[i].name) == 0) {
      return &builtins[i];
    }
  }
  return NULL;
}

int echo_fn(char **tokens, Redirection *r){
  bool save_for_restore = true;
  if (apply_redirection(r, save_for_restore) != 0) {
    return -1;
  }
  for(int i = 1; tokens[i] != NULL; i++){
    if (i > 1) {
      printf(" ");
    }
    printf("%s", tokens[i]);
  }
  printf("\n");
  if(r->saved_fd != -1){
    if (dup2(r->saved_fd, r->target_fd) == -1) {
      perror("dup2");
    }
    close(r->saved_fd);
    r->saved_fd = -1;
  }
  return 0;
}

int exit_fn(char **tokens, Redirection *r){
  exit(0);
  return 0;
}

int type_fn(char **tokens, Redirection *r){
  if(tokens[1] == NULL){
  return 0;
  }
  const Builtin *b = find_builtin(tokens[1], builtins, amount_builtins);
  if (b != NULL) {
    printf("%s is a shell builtin\n", tokens[1]);
    return 0;
  }
  char *path = check_if_executable(tokens[1]);
  if(path){
    printf("%s is %s\n", tokens[1], path);
    free(path);
  }
  else{
    printf("%s: not found\n", tokens[1]);
  }
  return 0;
}

int pwd_fn(char **tokens, Redirection *r){
  char cwd[1024];
  if(getcwd(cwd, sizeof(cwd)) != NULL){
  printf("%s\n", cwd);
  }
  else{
    perror("pwd");
  }
  return 0;
}
      
int cd_fn(char **tokens, Redirection *r){
  if(tokens[1] == NULL || strcmp(tokens[1], "~") == 0){
    chdir(getenv("HOME"));
  }
  else if(tokens[1] && chdir(tokens[1]) != 0){
    printf("cd: %s: No such file or directory\n", tokens[1]);
  }
  return 0;
}


int find_pipe_index(char **tokens){
  for(int i = 0; tokens[i] != NULL; i++){
    if(strcmp(tokens[i], "|")==0){
      return i;
    }
  }
  return -1;
}