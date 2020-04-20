#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char ** argv){
    struct termios old, new;
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON);
    new.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
    printf("Are you good?\n");
    char c = 0;
    if (read(0, &c, 1) == -1){
        perror("Read");
        return 1;
    }
    printf("\n");
    if (c == 'y')
        printf("I am happy\n");
    else if (c == 'n')
        printf("Its bad\n");
    else
        printf("I dont know what you said\n");
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return 0;
}
