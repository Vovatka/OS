#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#define BUF_SIZE 65536
#define MAX_FILENAME_LEN 255
#define HEADER_SIZE sizeof(struct file_header)
#define DEFAULT_FILE_MODE 0644
#define TEMPLATE_PATTERN "/tmp/archXXXXXX"

struct file_header
{
    char name[MAX_FILENAME_LEN + 1];
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t mtime;
    off_t size;
};

static ssize_t write_all(int fd, const void *buf, size_t count)
{
    const char *p = buf;
    size_t left = count;

    while (left > 0)
    {
        ssize_t w = write(fd, p, left);
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        left -= (size_t)w;
        p += w;
    }
    return (ssize_t)count;
}

static ssize_t read_all(int fd, void *buf, size_t count)
{
    char *p = buf;
    size_t left = count;

    while (left > 0)
    {
        ssize_t r = read(fd, p, left);
        if (r < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0)
            return (ssize_t)(count - left);
        left -= (size_t)r;
        p += r;
    }
    return (ssize_t)count;
}

void print_help(void)
{
    printf("Usage:\n");
    printf("./archiver arch_name -i(--input) file1 [file2 ...]\n");
    printf("./archiver arch_name -e(--extract) file1 [file2 ...]\n");
    printf("./archiver arch_name -s(--stat)\n");
    printf("./archiver -h(--help)\n");
}

int add_files(const char *arch_name, int file_count, char *files[])
{
    int arch_fd =
        open(arch_name, O_WRONLY | O_CREAT | O_APPEND, DEFAULT_FILE_MODE);
    if (arch_fd < 0)
        return -1;

    char buf[BUF_SIZE];

    for (int i = 0; i < file_count; ++i)
    {
        const char *filename = files[i];
        int infd = open(filename, O_RDONLY);
        if (infd < 0)
        {
            close(arch_fd);
            return -1;
        }

        struct stat st;
        if (fstat(infd, &st) != 0)
        {
            close(infd);
            close(arch_fd);
            return -1;
        }

        if (!S_ISREG(st.st_mode))
        {
            close(infd);
            continue;
        }

        struct file_header hdr;
        memset(&hdr, 0, HEADER_SIZE);
        strncpy(hdr.name, filename, MAX_FILENAME_LEN);
        hdr.name[MAX_FILENAME_LEN] = '\0';
        hdr.mode = st.st_mode;
        hdr.uid = st.st_uid;
        hdr.gid = st.st_gid;
        hdr.mtime = st.st_mtime;
        hdr.size = st.st_size;

        if (write_all(arch_fd, &hdr, HEADER_SIZE) != HEADER_SIZE)
        {
            close(infd);
            close(arch_fd);
            return -1;
        }

        off_t remaining = hdr.size;
        while (remaining > 0)
        {
            size_t chunk_size =
                (remaining > BUF_SIZE) ? BUF_SIZE : (size_t)remaining;
            ssize_t r = read(infd, buf, chunk_size);
            if (r < 0)
            {
                close(infd);
                close(arch_fd);
                return -1;
            }
            if (r == 0)
                break;

            if (write_all(arch_fd, buf, (size_t)r) != r)
            {
                close(infd);
                close(arch_fd);
                return -1;
            }
            remaining -= r;
        }
        close(infd);
    }

    close(arch_fd);
    return 0;
}

int list_archive(const char *arch_name)
{
    int afd = open(arch_name, O_RDONLY);
    if (afd < 0)
        return -1;

    struct file_header hdr;

    while (1)
    {
        ssize_t r = read_all(afd, &hdr, HEADER_SIZE);
        if (r == 0)
            break;
        if (r != HEADER_SIZE)
        {
            close(afd);
            return -1;
        }

        printf("%s\t%jd bytes\tmode:%o\tuid:%u\tgid:%u\tmtime:%jd\n", hdr.name,
               (intmax_t)hdr.size, (unsigned)hdr.mode, (unsigned)hdr.uid,
               (unsigned)hdr.gid, (intmax_t)hdr.mtime);

        if (lseek(afd, hdr.size, SEEK_CUR) == (off_t)-1)
        {
            close(afd);
            return -1;
        }
    }

    close(afd);
    return 0;
}

static int name_in_list(const char *name, int count, char *list[])
{
    for (int i = 0; i < count; ++i)
    {
        if (strcmp(name, list[i]) == 0)
            return 1;
    }
    return 0;
}

int extract_and_remove(const char *arch_name, int file_count, char *files[])
{
    int afd = open(arch_name, O_RDONLY);
    if (afd < 0)
        return -1;

    char tmp_template[] = TEMPLATE_PATTERN;
    int tmpfd = mkstemp(tmp_template);
    if (tmpfd < 0)
    {
        close(afd);
        return -1;
    }
    unlink(tmp_template);

    char buf[BUF_SIZE];
    int res = 0;

    while (1)
    {
        struct file_header hdr;
        ssize_t r = read_all(afd, &hdr, HEADER_SIZE);
        if (r == 0)
            break;
        if (r != HEADER_SIZE)
        {
            res = -1;
            break;
        }

        int to_extract = name_in_list(hdr.name, file_count, files);

        if (to_extract)
        {
            int outfd =
                open(hdr.name, O_WRONLY | O_CREAT | O_TRUNC, hdr.mode & 07777);
            if (outfd < 0)
            {
                res = -1;
                break;
            }

            off_t remaining = hdr.size;
            while (remaining > 0)
            {
                size_t chunk_size =
                    (remaining > BUF_SIZE) ? BUF_SIZE : (size_t)remaining;
                ssize_t rr = read_all(afd, buf, chunk_size);
                if (rr <= 0)
                {
                    close(outfd);
                    res = -1;
                    break;
                }

                if (write_all(outfd, buf, (size_t)rr) != rr)
                {
                    close(outfd);
                    res = -1;
                    break;
                }
                remaining -= rr;
            }

            if (res != 0)
            {
                close(outfd);
                break;
            }
            close(outfd);

            chown(hdr.name, hdr.uid, hdr.gid);
            chmod(hdr.name, hdr.mode & 07777);

            struct utimbuf times;
            times.actime = hdr.mtime;
            times.modtime = hdr.mtime;
            utime(hdr.name, &times);
        }
        else
        {
            if (write_all(tmpfd, &hdr, HEADER_SIZE) != HEADER_SIZE)
            {
                res = -1;
                break;
            }

            off_t remaining = hdr.size;
            while (remaining > 0)
            {
                size_t chunk_size =
                    (remaining > BUF_SIZE) ? BUF_SIZE : (size_t)remaining;
                ssize_t rr = read_all(afd, buf, chunk_size);
                if (rr <= 0)
                {
                    res = -1;
                    break;
                }

                if (write_all(tmpfd, buf, (size_t)rr) != rr)
                {
                    res = -1;
                    break;
                }
                remaining -= rr;
            }

            if (res != 0)
                break;
        }
    }

    if (res == 0)
    {
        if (close(afd) != 0)
        {
            close(tmpfd);
            return -1;
        }

        int afd2 =
            open(arch_name, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_MODE);
        if (afd2 < 0)
        {
            close(tmpfd);
            return -1;
        }

        if (lseek(tmpfd, 0, SEEK_SET) == (off_t)-1)
        {
            close(tmpfd);
            close(afd2);
            return -1;
        }

        while (1)
        {
            ssize_t rr = read(tmpfd, buf, BUF_SIZE);
            if (rr < 0)
            {
                if (errno == EINTR)
                    continue;
                close(tmpfd);
                close(afd2);
                return -1;
            }
            if (rr == 0)
                break;

            if (write_all(afd2, buf, (size_t)rr) != rr)
            {
                close(tmpfd);
                close(afd2);
                return -1;
            }
        }

        close(tmpfd);
        close(afd2);
    }
    else
    {
        close(afd);
        close(tmpfd);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        print_help();
        return 0;
    }

    if (argc < 3)
    {
        print_help();
        return 0;
    }

    const char *arch = argv[1];
    const char *cmd = argv[2];

    if (strcmp(cmd, "-i") == 0 || strcmp(cmd, "--input") == 0)
    {
        if (argc < 4)
        {
            print_help();
            return 1;
        }
        return add_files(arch, argc - 3, &argv[3]);
    }
    else if (strcmp(cmd, "-e") == 0 || strcmp(cmd, "--extract") == 0)
    {
        if (argc < 4)
        {
            print_help();
            return 1;
        }
        return extract_and_remove(arch, argc - 3, &argv[3]);
    }
    else if (strcmp(cmd, "-s") == 0 || strcmp(cmd, "--stat") == 0)
    {
        return list_archive(arch);
    }
    else
    {
        print_help();
        return 1;
    }
}
