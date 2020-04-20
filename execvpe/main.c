#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

extern char ** environ;

int execvpe(const char * file, char * const argv[], char * const env[]){
    for (int i = 0; env[i] != 0; ++i){
        putenv(env[i]);
    }
    execvp(file, argv);
    return 0;
}

int main(int argc, char ** argv){
    char * env[] = {"NEWvar=53", "MYVAR=Hello", 0};
    char * arg[] = {"test", "arg1", "arg2",0};
    execvpe("./test", arg, env);
    return 0;
}