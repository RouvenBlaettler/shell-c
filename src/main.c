#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  
  char input[256];

  // TODO: Uncomment the code below to pass the first stage
  while(input != "exit"){
  printf("$ ");
  fgets(input,sizeof(input), stdin);
  input[strlen(input)-1] = '\0';
  printf("hhasahs");
  printf("%s: command not found\n", input);
  }

  return 0;
}
