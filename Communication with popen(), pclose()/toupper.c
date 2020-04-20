#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char ** argv){
    char buffer[BUFFER_SIZE] = {0};
    int len = 0;
    while ((len = read(0, buffer, BUFFER_SIZE)) != 0){
        if (len == -1){
            perror("Read");
            return 1;
        }
        for (int i = 0; i < len; ++i){
            buffer[i] = toupper(buffer[i]);
        }
        printf("%s", buffer);
    }
    printf("\n");
    return 0;
}