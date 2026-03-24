#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>


char* check_if_executable(const char* cmd);
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
int find_pipe_index(char **tokens);
int validate_pipe_syntax(char **tokens, int pipe_idx);
int execute_pipe(char **tokens, int pipe_idx);


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
    int pipe_idx = find_pipe_index(tokens);
    if(validate_pipe_syntax(tokens, pipe_idx) != 0){
      printf("Wrong pipe syntax\n");
      free_tokens(tokens);
      continue;
    }
    int token_amount = token_count(tokens);
    if(tokens[0]==NULL){
      free_tokens(tokens);
      continue;
    }

    if (pipe_idx != -1) {
      execute_pipe(tokens, pipe_idx);
      free_tokens(tokens);
      continue;
    }

    if (parse_redirection(tokens, r, token_amount) != 0) {
      free_tokens(tokens);
      continue;
    }

    if (tokens[0] == NULL){   //after parse_redirection where < gets replaced with NULL a NULL-cmd could get created, e.g. > out.txt 
      fprintf(stderr, "syntax error: missing command before redirection\n");
      free_tokens(tokens);
      continue;
    }

    bool save_for_restore;

    const Builtin *b = find_builtin(tokens[0], builtins, amount_builtins);
    if (b != NULL) {
      save_for_restore = true;
      if (apply_redirection(r, save_for_restore) != 0) {
        free_tokens(tokens);
        continue;
      }
      int rc = b->function(tokens, r);
      if (rc == 1) break;   // optional exit code convention
      if(r->saved_fd != -1){
        if (dup2(r->saved_fd, r->target_fd) == -1) {
          perror("dup2");
        }
        close(r->saved_fd);
        r->saved_fd = -1;
      }
      free_tokens(tokens);
      
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

  char* p = input;

  while (*p != '\0') {

    while (*p == ' ' || *p == '\t') p++; //skips spaces and tabs //p++ makes that p points to next letter
    if (*p == '\0') break;

    // pipe is its own token
    if (*p == '|') {
      tokens[count++] = strdup("|"); //after assigning value to tokens[count] is count being increased
      p++; //use strdup("|") instead of "|" becuase that is a string lateral, so read only but we want dynamic mutable memory from the stack
      continue;
    }

    char buf[1024]; //stack buffer(tmp memory)
    int len = 0;

    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '|') {
      if (*p == '\'') {
        p++; // skip opening '
        while (*p != '\0' && *p != '\'') {
          buf[len++] = *p++; //copies charackter for charackter into buffer
        }
        if (*p == '\'') p++; // skip closing '
      } 
      else if (*p == '"') {
        p++; // skip opening "
        while (*p != '\0' && *p != '"') {
          if (*p == '\\' && (*(p+1) == '"' || *(p+1) == '\\' || *(p+1) == '$' || *(p+1) == '\n')) { //if \ infront of special char it doesn't have special meaning anymore and the \ gets omitted
            p++; // consume backslash, emit next char literally
          }
          buf[len++] = *p++;
        }
        if (*p == '"') p++; // skip closing "
      } 
      else if (*p == '\\' && *(p+1) != '\0') { //outside of quotes
        p++; // consume backslash
        buf[len++] = *p++; // emit next char literally
      } 
      else {
        buf[len++] = *p++;
      }
    }

    buf[len] = '\0';
    tokens[count++] = strdup(buf);
  }

  tokens[count] = NULL;
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
    } else if (strcmp(tokens[i], "<") == 0) {
      target_fd = 0; append = false;
    }else {
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

  int flags;
  if (r->target_fd == STDIN_FILENO) {
    flags = O_RDONLY;
  } else {
    flags = O_WRONLY | O_CREAT | (r->append ? O_APPEND : O_TRUNC);
  }
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
  for(int i = 1; tokens[i] != NULL; i++){
    if (i > 1) {
      printf(" ");
    }
    printf("%s", tokens[i]);
  }
  printf("\n");
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

int validate_pipe_syntax(char **tokens, int pipe_idx){
  if(pipe_idx == -1){
    return 0;
  }
  if(pipe_idx == 0 || tokens[pipe_idx + 1] == NULL){
    return -1;
  }

  for (int i = pipe_idx + 1; tokens[i] != NULL; i++){
    if(strcmp(tokens[i], "|")==0){
      if(tokens[i+1] != NULL && strcmp(tokens[i+1], "|")==0){
        return -1;
      }
    }
  }
  return 0;
}



int execute_pipe(char **tokens, int pipe_idx){
  char **first_tokens = tokens;
  char **last_tokens = &tokens[pipe_idx + 1];
  tokens[pipe_idx] = NULL;
  char **all_tokens[64];
   int i = 0;
  all_tokens[i++] = first_tokens;
  pipe_idx = find_pipe_index(last_tokens);
  while(pipe_idx != -1){
    all_tokens[i++] = last_tokens;
    last_tokens[pipe_idx] = NULL;
    last_tokens = &last_tokens[pipe_idx+1];
    pipe_idx = find_pipe_index(last_tokens);
  }
  all_tokens[i] = last_tokens;

  int cmd_amount = i + 1; //+2 because of left tokens and right tokens
  int pipe_amount = cmd_amount - 1;
  int pipe_fd[64][2];
  pid_t pids[64];


  for(int k = 0; k < pipe_amount; k++){
    if(pipe(pipe_fd[k]) ==  -1){
      perror("pipe");
      return -1;
    }
  }


  for(int j = 0; j < cmd_amount; j++){
    const Builtin *b = find_builtin(all_tokens[j][0], builtins, amount_builtins);
    char *path = check_if_executable(all_tokens[j][0]);
    if (path == NULL && !b) {
      printf("%s: command not found\n", all_tokens[j][0]);
      free(path);
      for (int k = 0; k < pipe_amount; k++) {
        close(pipe_fd[k][0]);
        close(pipe_fd[k][1]);
        }
      for (int k = 0; k < j; k++) {
        waitpid(pids[k], NULL, 0);
      }
      return -1;
    }
    

    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      free(path);
      for (int k = 0; k < pipe_amount; k++) {
        close(pipe_fd[k][0]);
        close(pipe_fd[k][1]);
      }
      for (int k = 0; k < j; k++) {
        waitpid(pids[k], NULL, 0);
      }
      return -1;
    }

    if(pid == 0){

      if(j == 0){
          if (dup2(pipe_fd[j][1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(1);
          }
      }
      else if(j == cmd_amount-1){
          if (dup2(pipe_fd[j-1][0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(1);
          }
      }
      else{
          if (dup2(pipe_fd[j-1][0], STDIN_FILENO) == -1 || dup2(pipe_fd[j][1], STDOUT_FILENO) == -1){
            perror("dup2");
            _exit(1);
          }
      }

      for (int k = 0; k < pipe_amount; k++) {
        close(pipe_fd[k][0]);
        close(pipe_fd[k][1]);
      }

      if(b){
        Redirection no_redir = {false, 1, false, NULL, -1};
        int rc = b->function(all_tokens[j], &no_redir);
        _exit(rc);
      }
      execv(path, all_tokens[j]);
      perror("execv");
      _exit(1);
    }
    pids[j] = pid;
    free(path);
  }
    // parent closes all pipes here
  for (int k = 0; k < pipe_amount; k++) {
    close(pipe_fd[k][0]);
    close(pipe_fd[k][1]);
  }

  // parent waits here
  for (int j = 0; j < cmd_amount; j++) {
    waitpid(pids[j], NULL, 0);
  }
  return 0;
}

