#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE 4096

void grep(const char *pattern, FILE *input, const char *filename)
{
    char buffer[BUFF_SIZE];

    while (fgets(buffer, sizeof(buffer), input) != NULL)
    {
        if (strstr(buffer, pattern) != NULL)
        {
            if (filename != NULL)
            {
                printf("%s:", filename);
            }
            printf("%s", buffer);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <pattern> [file1] [file2] ...\n", argv[0]);
        fprintf(stderr, "       or: <command> | %s <pattern>\n", argv[0]);
        return 1;
    }

    const char *pattern = argv[1];

    //if (isatty(fileno(stdin)) == 0)
    if (isatty(STDIN_FILENO) == 0)
    {
        grep(pattern, stdin, NULL);
    }
    else
    {
        if (argc > 2)
        {
            int success_count = 0;
            
            for (int i = 2; i < argc; i++)
            {
                FILE *file = fopen(argv[i], "r");
                if (file == NULL)
                {
                    perror(argv[i]);
                    continue;
                }
                
                grep(pattern, file, argv[i]);
                fclose(file);
                success_count++;
            }
            
            if (success_count == 0)
            {
                fprintf(stderr, "Error: Could not open any files\n");
                return 1;
            }
        }
        else
        {
            fprintf(stderr, "Usage: %s <pattern> [file1] [file2] ...\n", argv[0]);
            fprintf(stderr, "       or: <command> | %s <pattern>\n", argv[0]);
            return 1;
        }
    }

    return 0;
}
