#include "pthread.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "dirent.h"
#include "stddef.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "fcntl.h"

#define STRING_BUFFER_SIZE 1024
#define INVALID_ARGUMENTS -7
#define ERROR_CODE -1
#define FILE_BUFFER_SIZE 8

typedef struct paths{
    char * src;
    char * dest;
}paths_t;

void *cpFunction(void *arg);


paths_t * buildNewPath(paths_t * pArg, const char *additionalPath) {
    size_t additionalLen = strlen(additionalPath);
    size_t sourcePathLen = strlen(pArg->src) + additionalLen + 1;
    size_t destinationPathLen = strlen(pArg->dest) + additionalLen + 1;

    paths_t * p = (paths_t *)malloc(sizeof(paths_t));
    p->src = (char * )calloc(sourcePathLen, sizeof(char));
    p->dest = (char *)calloc(destinationPathLen, sizeof(char));

    strcpy(p->src, pArg->src);
    printf("src- %s\n", p->src);
    strcat(p->src, "/");
    strcat(p->src, additionalPath);

    strcpy(p->dest, pArg->dest);
    strcat(p->dest, "/");
    strcat(p->dest, additionalPath);
    return p;
}

void freePaths(paths_t *p) {
    free(p->src);
    free(p->dest);
    free(p);
}



int copyFolder(paths_t * p, mode_t mode) {
    DIR *dir;
    struct dirent *entry, *result;
    if (mkdir(p->dest, mode) == -1 && errno != EEXIST) {
        printf("Couldn't create directory");
        return ERROR_CODE;
    }
    if ((dir = opendir(p->src)) == NULL) {
        if (errno != EMFILE) {
            printf("Couldn't open directory");
            return ERROR_CODE;
        }
    }
    entry = (struct dirent *) malloc(sizeof(struct dirent));
    if (entry == NULL) {
        printf("Couldn't malloc for dirent");
        return ERROR_CODE;
    }
    while (readdir_r(dir, entry, &result) == 0 && result != NULL) {
        pthread_t thread;
        int pthreadCreateRetVal = 0;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
	entry->d_name[strlen(entry->d_name)] = 0;
        paths_t * newPaths = buildNewPath(p, entry->d_name);
        while ((pthreadCreateRetVal = pthread_create(&thread, NULL, cpFunction, (void *) newPaths)) && errno == EAGAIN) {}
        if (pthreadCreateRetVal != 0) {
            printf("Couldn't copy");
            free(newPaths);
        }
    }
    free(entry);
    if (closedir(dir) == -1)
        fprintf(stderr, "Couldn't close directory, %s\n", strerror(errno));

    return 0;
}

int copyFile(paths_t * p, mode_t mode) {
    int fdin, fdout;
    int bytesRead;
    char buf[FILE_BUFFER_SIZE];
    int returnValue = 0;

    if ((fdin = open(p->src, O_RDONLY)) == -1) {
        printf("Couldn't open file");
        return ERROR_CODE;
    }
    
    if((fdout = open(p->dest, O_WRONLY | O_CREAT | O_EXCL, mode)) == -1) {
        printf("Couldn't open file");
        if (close(fdin) != 0){
            printf("Couldn't close source file");
        }       
        return ERROR_CODE;
    }
    while ((bytesRead = read(fdin, buf, FILE_BUFFER_SIZE)) > 0) {
        char *writePtr = buf;
        int bytesWritten;
        while (bytesRead > 0){
            bytesWritten = write(fdout, writePtr, bytesRead);
            if (bytesWritten >= 0) {
                bytesRead -= bytesWritten;
                writePtr += bytesWritten;
            } else{
                returnValue = ERROR_CODE;
                break;
            }
        }
    }
    if (bytesRead < 0) {
        perror(strerror(errno));
        returnValue = ERROR_CODE;
    }
    if (close(fdin) != 0) {
        perror(strerror(errno));
        returnValue = ERROR_CODE;
    }
    if (close(fdout) != 0) {
        perror(strerror(errno));
        returnValue = ERROR_CODE;
    }
    return returnValue;
}


void *cpFunction(void *arg) {
    struct stat buf;
    paths_t * p = (paths_t *) arg;

    if (stat(p->src, &buf) != 0) {
        printf("Error: problem with stat %s\n", p->src);
        freePaths(p);
        return (void *) ERROR_CODE;
    }

    if (S_ISDIR(buf.st_mode))
        copyFolder(p, buf.st_mode);
    else if (S_ISREG(buf.st_mode))
        copyFile(p, buf.st_mode);
    freePaths(p);
}


paths_t * make_paths(char *src, char *dest) {
    struct stat buf;

    if (stat(src, &buf) == -1) {
        printf("Problem with filling stat");
        return NULL;
    }
    size_t len_source_path = strlen(src);
    size_t len_destination_path = strlen(dest);
    paths_t * p = (paths_t *)malloc(sizeof(paths_t));
    p->src = (char * )calloc(len_source_path, sizeof(char));
    p->dest = (char *)calloc(len_destination_path, sizeof(char));
    strncpy(p->src, src, len_source_path);
    strncpy(p->dest, dest, len_destination_path);
    if (p->src[len_source_path - 1] == '/'){
        p->src[len_source_path - 1] == 0;
	printf("1\n");
    }
    if (p->dest[len_destination_path - 1] == '/'){
        p->dest[len_destination_path - 1] == 0;
	printf("2\n");
    }
    return p;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error: invalid arguments!");
        exit(INVALID_ARGUMENTS);
    }
    paths_t * p = make_paths(argv[1], argv[2]);
    if (p == NULL) {
        exit(EXIT_FAILURE);
    }

    cpFunction(p);
    pthread_exit(NULL);
}

