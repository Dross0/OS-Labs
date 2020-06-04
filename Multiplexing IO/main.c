#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>

#define MAX_FILES 100
#define BUFFER_SIZE 1024
#define TIME 1

void close_file_array(FILE * files[], int count){
    for (int i = 0; i < count; ++i){
        fclose(files[i]);
    }
}

int main(int argc, char ** argv){
    if (argc <= 1){
        fprintf(stderr, "Wrong arguments amount");
        return 1;
    }
    FILE * files[MAX_FILES] = {0};
    int file_number = 0;
    for (; file_number < argc - 1 && file_number < MAX_FILES; ++file_number){
        if ((files[file_number] = fopen(argv[file_number + 1], "rb")) == NULL){
            close_file_array(files, file_number-1);
            perror("Cant open file");
            return 2;
        }
    }
    struct timeval timeout = {TIME, 0};
    fd_set readfds;
    char buffer[BUFFER_SIZE+1] = {0};
    while (file_number > 0){
        for (int i = 0; i < file_number; ++i){
            FD_ZERO(&readfds);
            int fdesc = fileno(files[i]);
            FD_SET(fdesc, &readfds);
            if (select(fdesc + 1, &readfds, NULL, NULL, &timeout) > 0){
                if (fgets(buffer, BUFFER_SIZE, files[i])){
                    int strSize = strlen(buffer);
                    write(1, buffer, strSize);
                }
                else{
                    fclose(files[i]);
                    for (int j = i + 1; j < file_number; ++j){
                        files[j-1] = files[j];
                    }
                    --file_number;
                }
            }

        }

    }
    return 0;
}