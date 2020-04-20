#include <stdio.h>
#include <stdlib.h>

extern char ** environ;

int main(int argc, char ** argv){
    for (int i = 0; environ[i] != 0; i++){
        printf("%s\n", environ[i]);
    }
    printf("ARGS\n");
    for (int i = 0; i < argc; ++i){
        printf("%s\n", argv[i]);
    }
    return 0;
}