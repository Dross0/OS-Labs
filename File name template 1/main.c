#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>


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


int main(int argc, char ** argv){
    if (argc != 2){
        fprintf(stderr, "Wrong arguments number");
        return -1;
    }
    DIR * dir = opendir(".");
    if (!dir){
        perror("opendir");
        return 2;
    }
    struct dirent * entry;
    while ((entry = readdir(dir)) != NULL){
        if (match_template(argv[1], entry->d_name)){
            printf("%s\n", entry->d_name);
        }
    }
    closedir(dir);
    return 0;
}