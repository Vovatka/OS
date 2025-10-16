#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define BUFF_SIZE 4096

int n_flag = 0;
int b_flag = 0;
int E_flag = 0;

void cat_func(FILE *file)
{
    char buffer[BUFF_SIZE];
    int lines = 1;
    int is_first_char = 1;
    int was_newline = 0;
    
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        size_t len = strlen(buffer);
        
        int has_newline = (len > 0 && buffer[len - 1] == '\n');
        if (has_newline)
        {
            buffer[len - 1] = '\0';
            len--;
        }
        
        if ((n_flag || b_flag) && is_first_char)
        {
            if (b_flag && len != 0)
            {
                printf("%6d\t", lines++);
            }
        }
        
        printf("%s", buffer);

        if (E_flag && has_newline)
        {
            printf("$");
        }
 
        if (has_newline)
        {
            printf("\n");
            was_newline = 1;
            is_first_char = 1;
        } 
        else
        {
            was_newline = 0;
            is_first_char = 0;
        }
    }

    if (!was_newline)
    {
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    FILE *file;
    int opt;

    while ((opt = getopt(argc, argv, "nbE")) != -1)
    {
        switch (opt)
        {
            case 'n':
                n_flag = 1;
                break;
            case 'b':
                b_flag = 1;
                break;
            case 'E':
                E_flag = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n] [-b] [-E] [file...]\n", argv[0]);
                return 1;
        }
    }

    if (n_flag && b_flag)
    {
        n_flag = 0;
    }

    for (int i = optind; i < argc; i++) 
    {
        if (strcmp(argv[i], "-") == 0)
        {
              cat_func(stdin);
        } 
        else
        {
            file = fopen(argv[i], "r");
            if (file == NULL)
            {
                perror(argv[i]);
                continue;
            }
            cat_func(file);
            fclose(file);
        }
    }
    return 0;
}
