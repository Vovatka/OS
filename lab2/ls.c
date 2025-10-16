#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <limits.h> //*_MAX

#define MAX_FILES 1024

struct FileInfo
{
    char name[NAME_MAX];
    struct stat st;
    char owner_name[32];
    char group_name[32];
    char link_target[PATH_MAX];
};

int compare_files(const void *f1, const void *f2)
{
    const struct FileInfo *file1 = (const struct FileInfo *)f1;
    const struct FileInfo *file2 = (const struct FileInfo *)f2;
    return strcmp(file1->name, file2->name);
}

void get_mode_string(mode_t mode, char* buf)
{
    if (S_ISDIR(mode)) buf[0] = 'd';
    else if (S_ISLNK(mode)) buf[0] = 'l';
    else if (S_ISCHR(mode)) buf[0] = 'c';
    else if (S_ISBLK(mode)) buf[0] = 'b';
    else if (S_ISFIFO(mode)) buf[0] = 'p';
    else if (S_ISSOCK(mode)) buf[0] = 's';
    else buf[0] = '-';

    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';

    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';

    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';

    buf[10] = '\0';
}

const char* file_color(const struct stat* st)
{
    if (S_ISDIR(st->st_mode)) return "\033[1;34m";
    if (S_ISLNK(st->st_mode)) return "\033[1;36m";
    if (st->st_mode & S_IXUSR) return "\033[1;32m";
    return "\033[0m";
}

void format_time(time_t t, char* buffer, size_t size)
{
    struct tm* timeinfo = localtime(&t);
    strftime(buffer, size, "%b %d %H:%M", timeinfo);
}

void print_simple_format(struct FileInfo* files, int count)
{
    int max_name_len = 0;
    for (int i = 0; i < count; i++)
    {
        int len = strlen(files[i].name);
        if (len > max_name_len) max_name_len = len;
    }
    
    struct winsize ws;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
    { 
        term_width = ws.ws_col;
    }
    
    int col_width = max_name_len + 2;
    int cols = term_width / col_width;
    if (cols == 0) cols = 1;
    
    for (int i = 0; i < count; i++)
    {
        const char* color = file_color(&files[i].st);
        printf("%s%-*s\033[0m", color, col_width, files[i].name);
        
        if ((i + 1) % cols == 0 || i == count - 1)
        {
            printf("\n");
        }
    }
}

void print_long_format(struct FileInfo* files, int count)
{
    long long total_blocks = 0;
    
    for (int i = 0; i < count; i++)
    {
        total_blocks += files[i].st.st_blocks;
    }
    
    printf("total %lld\n", total_blocks / 2);
    
    for (int i = 0; i < count; i++)
    {
        char modes[11];
        char time_str[32];
        
        get_mode_string(files[i].st.st_mode, modes);
        format_time(files[i].st.st_mtime, time_str, sizeof(time_str));
        const char* color = file_color(&files[i].st);
        
        printf("%s %2lu %-8s %-8s %8lld %s %s%s\033[0m",
               modes,
               (unsigned long)files[i].st.st_nlink,
               files[i].owner_name,
               files[i].group_name,
               (long long)files[i].st.st_size,
               time_str,
               color,
               files[i].name);
        
        if (S_ISLNK(files[i].st.st_mode) && files[i].link_target[0] != '\0')
        {
            printf(" -> %s", files[i].link_target);
        }
        printf("\n");
    }
}

void list_dir(const char* path, bool flag_a, bool flag_l, bool print_header) {
    DIR* dp;
    struct dirent* entry;
    struct FileInfo files[MAX_FILES];
    int count = 0;
    
    dp = opendir(path);
    if (dp == NULL)
    {
        perror(path);
        return;
    }
    
    while ((entry = readdir(dp)) != NULL && count < MAX_FILES) {
        if (!flag_a && entry->d_name[0] == '.')
        {
            continue;
        }
        
        strncpy(files[count].name, entry->d_name, sizeof(files[count].name) - 1);
        files[count].name[sizeof(files[count].name) - 1] = '\0';
        
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        
        if (lstat(fullpath, &files[count].st) == -1)
        {
            continue;
        }
        
        struct passwd* pwd = getpwuid(files[count].st.st_uid);
        if (pwd != NULL)
        {
            strncpy(files[count].owner_name, pwd->pw_name, sizeof(files[count].owner_name) - 1);
        }
        else
        {
            snprintf(files[count].owner_name, sizeof(files[count].owner_name), "%d", files[count].st.st_uid);
        }
        
        struct group* grp = getgrgid(files[count].st.st_gid);
        if (grp != NULL)
        {
            strncpy(files[count].group_name, grp->gr_name, sizeof(files[count].group_name) - 1);
        }
        else
        {
            snprintf(files[count].group_name, sizeof(files[count].group_name), "%d", files[count].st.st_gid);
        }
        
        files[count].link_target[0] = '\0';
        if (S_ISLNK(files[count].st.st_mode))
        {
            ssize_t len = readlink(fullpath, files[count].link_target, sizeof(files[count].link_target) - 1);
            if (len != -1)
            {
                files[count].link_target[len] = '\0';
            }
        }
        
        count++;
    }
    
    closedir(dp);
    
    qsort(files, count, sizeof(struct FileInfo), compare_files);
    
    if (print_header)
    {
        printf("%s:\n", path);
    }
    

    if (flag_l)
    {
        print_long_format(files, count);
    }
    else
    {
        print_simple_format(files, count);
    }
}

int main(int argc, char* argv[]) {
    bool flag_a = false;
    bool flag_l = false;
    int opt;
    
    while ((opt = getopt(argc, argv, "al")) != -1)
    {
        switch (opt)
        {
            case 'a':
                flag_a = true;
                break;
            case 'l':
                flag_l = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-a] [-l] [directory...]\n", argv[0]);
                return 1;
        }
    }
    
    if (optind == argc)
    {
        list_dir(".", flag_a, flag_l, false);
    } 
    else
    {
        for (int i = optind; i < argc; i++)
        {
            list_dir(argv[i], flag_a, flag_l, (argc - optind > 1));
            if (i < argc - 1)
            {
                printf("\n");
            }
        }
    }
    
    return 0;
}
