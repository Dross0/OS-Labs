#include <stdio.h>
#include <ctype.h>
#include <stdio.h>

#define BUFFER_SIZE 1024

int main(int argc, char ** argv){
    FILE * writer = 0;
    FILE * toupp = 0;
    if ((writer = popen("./writer JrufneUBU", "r")) == NULL){
        perror("Writer");
        return 1;
    }
    if ((toupp = popen("./toupper", "w")) == NULL){
        perror("Toupper");
        return 2;
    }
    char buffer[BUFFER_SIZE] = {0};
    int len = 0;
    while (fgets(buffer, BUFFER_SIZE, writer)){
        fwrite(buffer, sizeof(char), BUFFER_SIZE, toupp);
    }
    pclose(writer);
    pclose(toupp);
    return 0;
}