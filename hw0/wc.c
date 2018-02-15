#include <stdio.h>
#include <stdbool.h>

// count the line, word, and characters from a file or stdin
int main(int argc, char *argv[]) {
    if (argc > 2) {
        printf("Too many files, I cant handle that just yet");
        return -1;
    } 

    FILE* fin;
    char* fileName;
    if (argc == 1) {
        fin = stdin;
    } else {
        fileName = argv[1];
        fin = fopen(fileName, "r");
        if (fin == NULL) {
            printf("invalid file");
            return -2;
        }
    }

    int linec = 0, wordc = 0, charc = 0; // init line count, word count, and char count to 0
    bool counting = false;
    int c;
    while ((c = fgetc(fin)) != EOF){
        switch (c) {
        case '\n':
            ++linec;
            wordc += counting;
            counting = false;
            break;

        case '\t':
        case '\0':
        case ' ':
            if (counting) {
                ++wordc;
                counting = false;
            }
            break;

        default:
            counting = true;
            break;
        }
        ++charc;
    }
    wordc += counting;
    
    if (argc == 1) {
        printf("\t%i\t%i\t%i\n", linec, wordc, charc);
    } else {
        printf(" %i %i %i %s\n", linec, wordc, charc, fileName);
    }
    
    
    return 0;
}
