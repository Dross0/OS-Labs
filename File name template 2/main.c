#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int match_template(const char * template, char * str){
    size_t strSize = strlen(str);
    char ** startCharArray = (char **)malloc(sizeof(char *) * strSize);
    size_t startCharArrayIndex = 0; 
    size_t templateIndex = 0;
    size_t templateSize = strlen(template);
    size_t i = 0;
    for (i = 0; i < strSize && templateIndex < templateSize; ++i){
        if (template[templateIndex] == '?'){
            templateIndex++;
            continue;
        }
        else if (template[templateIndex] == '/'){
            return -1;
        }
        else if (template[templateIndex] == '*'){
            if (templateIndex == templateSize - 1){
                free(startCharArray);
                return 1;
            }
            else if (template[templateIndex + 1] == '?'){
                for (size_t j = i; j < strSize; ++j){
                    startCharArray[startCharArrayIndex++] = str + j;
                }
            }
            else if (template[templateIndex + 1] != '*'){
                char nextChar = template[templateIndex + 1];
                for (size_t j = i; j < strSize; ++j){
                    if (str[j] == nextChar){
                        startCharArray[startCharArrayIndex++] = str + j;
                    }
                }
            }
            for (size_t j = 0; j < startCharArrayIndex; ++j){
                if (match_template(template + templateIndex + 1, startCharArray[j]) == 1){
                    free(startCharArray);
                    return 1;
                }
            }
            free(startCharArray);
            return 0;
        }
        else {
            if (template[templateIndex] != str[i]){
                free(startCharArray);
                return 0;
            }
        }
        templateIndex++;
    }
    free(startCharArray);
    if (templateIndex == templateSize && i != strSize){
        return 0;
    }
    if (i == strSize){
        for (; templateIndex < templateSize; templateIndex++){
            if (template[templateIndex] != '*'){
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

int isDir(char * path){
    struct stat buf;
    if (lstat(path, &buf) == -1){
        perror("stat");
        exit(2);
    }
    return (buf.st_mode & S_IFMT) == S_IFDIR;
}

void print_templates_files(DIR * dir, char * dirPath, char * template, int identation_lvl){
    if (template == NULL){
        return;
    }
    chdir(dirPath);
    char * last = NULL;
    char * next_template = strtok_r(template, "/", &last);
    struct dirent * entry;
    while ((entry = readdir(dir)) != NULL){
        if (match_template(next_template, entry->d_name)){
            for (int i = 0; i < identation_lvl * 2; ++i){
                printf("%c", ' ');
            }
            printf("%s\n", entry->d_name);
            if (!strcmp(entry->d_name, "..") || !strcmp(entry->d_name, ".")){
                continue;
            }
            if (isDir(entry->d_name)){
                DIR * newDir = opendir(entry->d_name);
                if (!newDir){
                    perror("openDir");
                    exit(3);
                }
                print_templates_files(newDir, entry->d_name, last, identation_lvl + 1);
                closedir(newDir);
            }
        }
    }
    chdir("..");
}



int main(int argc, char ** argv){
    if (argc != 2){
        fprintf(stderr, "Wrong arguments number");
        return -1;
    }
    char sep[] = "/";
    char tmp[1024] = {0};
    strncpy(tmp, argv[1], 1024);
    char * last;
    char * start_path = strtok_r(argv[1], sep, &last);
    char * template = tmp;
    DIR * dir = NULL;
    char * dirPath = NULL;
    if (!strcmp(start_path, ".")){
        dirPath = start_path;
        dir = opendir(start_path);
        template = last;
    }
    else if (tmp[0] == '/'){
        dir = opendir("/");
        dirPath = "/";
    }
    else {
        dir = opendir("."); 
        dirPath = ".";
    }
    if (!dir){
        perror("opendir");
        return 2;
    }
    print_templates_files(dir, dirPath, template, 0);
    closedir(dir);
    return 0;
}