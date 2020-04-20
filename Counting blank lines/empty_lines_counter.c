#include <stdlib.h>
#include <stdio.h>

int count_empty_lines(char * file_name){
	char buffer[1024] = {0};
	snprintf(buffer, 1024, "grep \"^\\s*$\" %s | wc -l > tmpfile", file_name);
	system(buffer);
	FILE * f = fopen("tmpfile", "r");
	int empty_lines = 0;
	fscanf(f, "%d", &empty_lines);
	fclose(f);
	system("rm -f tmpfile");
	return empty_lines;
}

int main(int argc, char ** argv){
	if (argc != 2){
		fprintf(stderr, "Wrong arguments number\n");
		return 1;
	}
	int empty_lines = count_empty_lines(argv[1]);
	printf("Empty lines in \"%s\" = %d\n", argv[1], empty_lines);
	return 0;
}