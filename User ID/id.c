#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char ** argv){
	printf("Real = %d\n", getuid());
	printf("Effective = %d\n", geteuid());
    FILE * file = fopen("file", "r");
    if (file == NULL){
        perror("");
    } 
    else {
        printf("FINE\n");
        fclose(file);
    }                                                
	setuid(getuid());
	printf("Real = %d\n", getuid());
    printf("Effective = %d\n", geteuid());
    FILE * f = fopen("file", "r");
	if (f == NULL){
		perror("");
	}
	else {
		printf("FINE\n");
        fclose(f);
	}
	return 0;
}
