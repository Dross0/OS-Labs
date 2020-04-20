#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

int main(int argc, char ** argv){
    FILE * fides[2];
    p2open("/bin/sort", fides);
    srand(time(NULL));
    for (int i = 0; i < 100; ++i){
        int random_num = rand() % 100;
        fprintf(fides[0], "%d\n", random_num);
    }
    fclose(fides[0]);
    char buf[10] = {0};
    while (fgets(buf, 10, fides[1]) != NULL){
        printf("%s\n", buf);
    }
    return 0;
}