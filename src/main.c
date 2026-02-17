#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  
  char cmd[256];

  // TODO: Uncomment the code below to pass the first stage
  printf("$ ");
  scanf("%s", &cmd);
  printf("%s: command not found\n", cmd);
  

  return 0;
}
