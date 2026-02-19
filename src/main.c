#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  
  char input[256];

  // TODO: Uncomment the code below to pass the first stage
  printf("$ ");
  fgets(input,sizeof(input), stdin);
  input[strlen(input)-1] = '\0';

  while(strcmp(input, "exit")){
    char *space = strchr(input, ' ');
    if(space != NULL){
      *space = '\0';
      char *first = input;
      char *rest = space + 1;
      if(strstr(first, "echo")){
        printf("%s\n", rest);
      }
      else{
        printf("%s: command not found\n", input);
      }
    }
    else{
    printf("%s: command not found\n", input);
    }
    printf("$ ");
    fgets(input,sizeof(input), stdin);
    input[strlen(input)-1] = '\0';
  }

  return 0;
}
