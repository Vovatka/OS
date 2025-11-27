#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define REQUIRED_ARGS_COUNT 3
#define OCTAL_BASE 8
#define MAX_OCTAL_MODE 07777
#define MIN_OCTAL_MODE 0

#define SYMBOLIC_USER 'u'
#define SYMBOLIC_GROUP 'g'
#define SYMBOLIC_OTHER 'o'
#define SYMBOLIC_ALL 'a'

#define OPERATOR_ADD '+'
#define OPERATOR_REMOVE '-'
#define OPERATOR_SET '='

#define PERMISSION_READ 'r'
#define PERMISSION_WRITE 'w'
#define PERMISSION_EXECUTE 'x'

#define CLAUSE_SEPARATOR ','

static void print_usage(const char *program_name)
{
    fprintf(stderr,
            "Usage: %s <mode> <file>\n", program_name);
}

static int is_octal_mode(const char *str)
{
    if (!*str)
        return 0;
    for (const char *ptr = str; *ptr; ++ptr)
    {
        if (*ptr < '0' || *ptr > '7')
            return 0;
    }
    return 1;
}

static int parse_octal_mode(const char *str, mode_t *result)
{
    char *end_ptr = NULL;
    errno = 0;
    long value = strtol(str, &end_ptr, OCTAL_BASE);

    if (errno != 0 || end_ptr == str || *end_ptr != '\0' || value < MIN_OCTAL_MODE ||
        value > MAX_OCTAL_MODE)
    {
        return -1;
    }

    *result = (mode_t)value;
    return 0;
}

typedef struct
{
    int user;
    int group;
    int other;
} PermissionTarget;

static void apply_permissions(mode_t *mode, PermissionTarget target,
                              char operation, int read_flag, int write_flag,
                              int execute_flag)
{
    mode_t permissions_to_add = 0;

    if (read_flag)
    {
        if (target.user)
            permissions_to_add |= S_IRUSR;
        if (target.group)
            permissions_to_add |= S_IRGRP;
        if (target.other)
            permissions_to_add |= S_IROTH;
    }

    if (write_flag)
    {
        if (target.user)
            permissions_to_add |= S_IWUSR;
        if (target.group)
            permissions_to_add |= S_IWGRP;
        if (target.other)
            permissions_to_add |= S_IWOTH;
    }

    if (execute_flag)
    {
        if (target.user)
            permissions_to_add |= S_IXUSR;
        if (target.group)
            permissions_to_add |= S_IXGRP;
        if (target.other)
            permissions_to_add |= S_IXOTH;
    }

    switch (operation)
    {
    case OPERATOR_SET:
        if (target.user)
            *mode &= ~(S_IRUSR | S_IWUSR | S_IXUSR);
        if (target.group)
            *mode &= ~(S_IRGRP | S_IWGRP | S_IXGRP);
        if (target.other)
            *mode &= ~(S_IROTH | S_IWOTH | S_IXOTH);
        *mode |= permissions_to_add;
        break;

    case OPERATOR_ADD:
        *mode |= permissions_to_add;
        break;

    case OPERATOR_REMOVE:
        *mode &= ~permissions_to_add;
        break;
    }
}

static int parse_and_apply_symbolic(const char *expression, mode_t *mode)
{
    const char *current_pos = expression;

    while (*current_pos)
    {
        PermissionTarget target = {0, 0, 0};
        int target_specified = 0;

        while (*current_pos == SYMBOLIC_USER || *current_pos == SYMBOLIC_GROUP ||
               *current_pos == SYMBOLIC_OTHER || *current_pos == SYMBOLIC_ALL)
        {
            target_specified = 1;

            switch (*current_pos)
            {
            case SYMBOLIC_USER:
                target.user = 1;
                break;
            case SYMBOLIC_GROUP:
                target.group = 1;
                break;
            case SYMBOLIC_OTHER:
                target.other = 1;
                break;
            case SYMBOLIC_ALL:
                target.user = target.group = target.other = 1;
                break;
            }
            ++current_pos;
        }

        if (!target_specified)
        {
            target.user = target.group = target.other = 1;
        }

        if (*current_pos != OPERATOR_ADD && *current_pos != OPERATOR_REMOVE && *current_pos != OPERATOR_SET)
        {
            fprintf(stderr, "Invalid operator near: %s\n", current_pos);
            return -1;
        }

        char operation = *current_pos++;

        if (*current_pos == '\0')
        {
            if (operation == OPERATOR_SET)
            {
                if (target.user)
                    *mode &= ~(S_IRUSR | S_IWUSR | S_IXUSR);
                if (target.group)
                    *mode &= ~(S_IRGRP | S_IWGRP | S_IXGRP);
                if (target.other)
                    *mode &= ~(S_IROTH | S_IWOTH | S_IXOTH);
                break;
            }
            fprintf(stderr, "Missing permissions after operator\n");
            return -1;
        }

        int read_permission = 0, write_permission = 0, execute_permission = 0;
        int any_permission_specified = 0;

        while (*current_pos == PERMISSION_READ || *current_pos == PERMISSION_WRITE ||
               *current_pos == PERMISSION_EXECUTE)
        {
            any_permission_specified = 1;

            switch (*current_pos)
            {
            case PERMISSION_READ:
                read_permission = 1;
                break;
            case PERMISSION_WRITE:
                write_permission = 1;
                break;
            case PERMISSION_EXECUTE:
                execute_permission = 1;
                break;
            }
            ++current_pos;
        }

        if (!any_permission_specified && operation != OPERATOR_SET)
        {
            fprintf(stderr, "Missing permission set in clause\n");
            return -1;
        }

        apply_permissions(mode, target, operation, read_permission,
                          write_permission, execute_permission);

        if (*current_pos == CLAUSE_SEPARATOR)
        {
            ++current_pos;
            if (*current_pos == '\0')
            {
                fprintf(stderr, "Trailing comma\n");
                return -1;
            }
        }
        else if (*current_pos != '\0')
        {
            fprintf(stderr, "Invalid characters near: %s\n", current_pos);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != REQUIRED_ARGS_COUNT)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *mode_string = argv[1];
    const char *file_path = argv[2];

    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0)
    {
        perror("stat");
        return EXIT_FAILURE;
    }

    mode_t new_mode = file_stat.st_mode;

    if (is_octal_mode(mode_string))
    {
        if (parse_octal_mode(mode_string, &new_mode) != 0)
        {
            fprintf(stderr, "Invalid octal mode: %s\n", mode_string);
            return EXIT_FAILURE;
        }
    }
    else
    {
        if (parse_and_apply_symbolic(mode_string, &new_mode) != 0)
        {
            return EXIT_FAILURE;
        }
    }

    if (chmod(file_path, new_mode) != 0)
    {
        perror("chmod");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
