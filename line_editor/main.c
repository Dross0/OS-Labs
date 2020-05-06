#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <ctype.h>

#define MAX_LINE_SIZE 40

typedef struct line{
    char * data;
    size_t size;
} line_t;

void line_init(line_t * line){
    line->data = (char *)calloc(MAX_LINE_SIZE + 1, sizeof(char));
    line->size = 0;
}

void line_append(line_t * line, char symbol){
    write(STDOUT_FILENO, &symbol, sizeof(char));
    line->data[line->size++] = symbol;
}

void new_line(line_t * line){
    write(STDOUT_FILENO, "\n", sizeof(char));
    line->size = 0;
}

void free_line(line_t * line){
    free(line->data);
    line->data = NULL;
    free(line);
}

void erase(line_t * line, int count){
    for (int i = line->size - 1, j = 0; j < count && i >= 0; j++, i--){
        line->size--;
        write(STDOUT_FILENO, "\b \b", sizeof(char) * 3);
    }
}

void word_erase(line_t * line){
    int space_pos = -1;
    int wasWord = 0;
    for (int i = line->size - 1; i >= 0; --i){
        if (isspace(line->data[i])){
            space_pos = i;
            if (wasWord){
                break;
            }
            space_pos = -1;
        }
        else{
            wasWord = 1;
        }
        
    }
    erase(line, line->size - space_pos - 1);
}

void wrap_word(line_t * line){
    int word_start = 0;
    for (word_start = line->size - 1; word_start >= 0; --word_start){
        if (isspace(line->data[word_start])){
            break;
        }
    }
    if (word_start >= 0){
        size_t prev_size = line->size;
        erase(line, line->size - word_start - 1);
        new_line(line);
        for (int i = word_start + 1; i < prev_size; ++i){
            line_append(line, line->data[i]);
        }
    }
    else{
        new_line(line);
    }
}


int main(int argc, char ** argv){
    struct termios old_set;
    if (tcgetattr(fileno(stdin), &old_set) == -1){
        perror("tcgetattr");
        return 1;
    }
    struct termios new_set = old_set;
    new_set.c_lflag &= ~(ECHO | ICANON);
    new_set.c_cc[VMIN] = 1;
    if (tcsetattr(fileno(stdin), TCSANOW, &new_set) == -1){
        perror("tcsetattr");
        return 2;
    }
    char character = 0;
    line_t * line = (line_t *)malloc(sizeof(line));  
    line_init(line);
    while (1){
        if (read(fileno(stdin), &character, sizeof(char)) == -1){
            perror("read");
            return 3;
        }
        if (new_set.c_cc[VERASE] == character){
            erase(line, 1);
        }
        else if (new_set.c_cc[VKILL] == character){
            erase(line, line->size);
        }
        else if (new_set.c_cc[VWERASE] == character){
            word_erase(line);
        }
        else if (new_set.c_cc[VEOF] == character){
            if (line->size == 0){
                break;
            }
        }
        else if (character == '\n'){
            new_line(line);
        }
        else if (isprint(character)){
            if (line->size == MAX_LINE_SIZE){
                if (isspace(character)){
                    new_line(line);
                }
                else{
                    wrap_word(line);
                }
            }
            line_append(line, character);
        }
        else{
            write(STDOUT_FILENO, "\a", sizeof(char));
        }

    }
    tcsetattr(fileno(stdin), TCSANOW, &old_set);
    free_line(line);
}