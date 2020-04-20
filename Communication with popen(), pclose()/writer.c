#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char ** argv){
    if (argc != 2){
        fprintf(stderr, "Wrong arguments number\n");
        return 1;
    }
    if (write(1, argv[1], strlen(argv[1])) == -1){
        perror("Write");
        return 2;
    }
    return 0;
}