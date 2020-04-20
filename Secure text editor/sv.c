#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, char ** argv){
	if (argc != 2){
		fprintf(stderr, "Wrong arguments number\n");
		return 1;
	}
	int fd = open(argv[1], O_RDWR);
    if (fd == -1){
        perror("open");
        return 1;
    }
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(fd, F_SETLK, &lock) == -1){
		perror("fcntl");
        return 2;
	}
	char buffer[1024] = {0};
	snprintf(buffer, 1024, "vi %s", argv[1]);
	system(buffer);
	close(fd);
	return 0;
}
