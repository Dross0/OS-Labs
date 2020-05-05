#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ulimit.h>

void print_uid_gid(){
    printf("Real user id = %d\nEffective user id = %d\nReal group id = %d\nEffective group id = %d\n", getuid(),
        geteuid(),
        getgid(),
        getgid());
}

void set_group_leader(){
    setpgid(0, getpgrp());
}

void print_proc_parent_group_id(){
    printf("Process id = %d\nParent process id = %d\nGroup process id = %d\n",
            getpid(),
            getppid(),
            getpgrp());
}

void print_ulimit(){
    printf("Ulimit = %d\n", ulimit(UL_GETFSIZE));
}

long set_ulimit(char * arg){
    long new_value = atol(arg);
    return ulimit(UL_SETFSIZE, new_value);
}

void print_core_file_size(){
    system("ulimit -c");
}

void print_pwd(){
    system("pwd");
}

extern char ** environ;

void print_envs(){
    int i = 0;
    while (environ[i] != NULL){
        printf("%s\n", environ[i]);
        i++;
    }
}

void set_core_file_size(char * arg){
    int value = atoi(arg);
    char buf[1024] = {0};
    snprintf(buf, 1024, "ulimit -c %d", value);
    system(buf);
    printf("%s\n", buf);
}

void set_env(char * optarg){
    char name[1024] = {0};
    char value[1024] = {0};
    int i = 0;
    int j = 0;
    int flag = 0;
    while (optarg[i] && i < 1024){
        if (optarg[i] == '='){
            flag = 1;
            j = 0;
        }
        else if (flag){
            value[j++] = optarg[i];
        }
        else {
            name[j++] = optarg[i];
        }
        i++;
    }
    setenv(name, value, 0);
}

int main(int argc, char ** argv){
    if (argc <= 1){
        fprintf(stderr, "No arguments\n");
    }
    char * opt = "ispuU:cC:dvV:"; 
    int res = 0;
    while((res = getopt(argc, argv, opt)) != -1){
        switch (res){
            case 'i':
                print_uid_gid();
                break;
            case 's':
                set_group_leader();
                break;
            case 'p':
                print_proc_parent_group_id();
                break;
            case 'u':
                print_ulimit();
                break;
            case 'U':
                set_ulimit(optarg);
                break;
            case 'c':
                print_core_file_size();
                break;
            case 'C':
                set_core_file_size(optarg);
                break;
            case 'd':
                print_pwd();
                break;
            case 'v':
                print_envs();
                break;
            case 'V':
                set_env(optarg);
                break;
            case '?':
                fprintf(stderr, "Invalid argument\n");
                return 2;
        }
    }
    return 0;
}
