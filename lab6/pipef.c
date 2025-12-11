#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TIMESTAMP_BUFFER_SIZE 128
#define MESSAGE_BUFFER_SIZE 256
#define FIFO_BUFFER_SIZE 512
#define FIFO_PATH "/tmp/myfifo"
#define FIFO_PERMISSIONS 0666

#define PIPE_DELAY_SECONDS 3
#define FIFO_DELAY_SECONDS 11

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

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

static void format_timestamp(time_t t, char *buffer, size_t buffer_size)
{
    ctime_r(&t, buffer);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }
}

int runPipe(void)
{
    int fds[2];
    if (pipe(fds) != 0)
        return EXIT_FAILURE;

    pid_t pid = fork();

    if (pid < 0)
    {
        close(fds[0]);
        close(fds[1]);
        return EXIT_FAILURE;
    }

    if (pid > 0)
    {
        close(fds[0]);

        time_t current_time = time(NULL);
        char timestamp[TIMESTAMP_BUFFER_SIZE];
        format_timestamp(current_time, timestamp, sizeof(timestamp));

        char message[MESSAGE_BUFFER_SIZE];
        int message_length =
            snprintf(message, sizeof(message), "PARENT pid=%d time=%s\n",
                     (int)getpid(), timestamp);

        write_all(fds[1], &message_length, sizeof(message_length));
        write_all(fds[1], message, (size_t)message_length);

        close(fds[1]);
        waitpid(pid, NULL, 0);
        return EXIT_SUCCESS;
    }
    else
    {
        close(fds[1]);

        int message_length;
        if (read_all(fds[0], &message_length, sizeof(message_length)) !=
            sizeof(message_length))
        {
            close(fds[0]);
            return EXIT_FAILURE;
        }

        char *message_buffer = malloc((size_t)message_length + 1);
        if (!message_buffer)
        {
            close(fds[0]);
            return EXIT_FAILURE;
        }

        if (read_all(fds[0], message_buffer, (size_t)message_length) !=
            message_length)
        {
            free(message_buffer);
            close(fds[0]);
            return EXIT_FAILURE;
        }
        message_buffer[message_length] = '\0';

        sleep(PIPE_DELAY_SECONDS);

        time_t child_time = time(NULL);
        char child_timestamp[TIMESTAMP_BUFFER_SIZE];
        format_timestamp(child_time, child_timestamp, sizeof(child_timestamp));

        printf("CHILD pid=%d current_time=%s\nRECEIVED: %s\n", (int)getpid(),
               child_timestamp, message_buffer);

        free(message_buffer);
        close(fds[0]);
        return EXIT_SUCCESS;
    }
}

int runFifoWrite(const char *fifo_path)
{
    int fd = open(fifo_path, O_WRONLY);
    if (fd < 0)
        return EXIT_FAILURE;

    time_t current_time = time(NULL);
    char timestamp[TIMESTAMP_BUFFER_SIZE];
    format_timestamp(current_time, timestamp, sizeof(timestamp));

    char message[FIFO_BUFFER_SIZE];
    int message_length =
        snprintf(message, sizeof(message), "WRITER pid=%d time=%s\n",
                 (int)getpid(), timestamp);

    write_all(fd, &message_length, sizeof(message_length));
    write_all(fd, message, (size_t)message_length);

    close(fd);
    return EXIT_SUCCESS;
}

int runFifoRead(const char *fifo_path)
{
    int fd = open(fifo_path, O_RDONLY);
    if (fd < 0)
        return EXIT_FAILURE;

    int message_length;
    if (read_all(fd, &message_length, sizeof(message_length)) !=
        sizeof(message_length))
    {
        close(fd);
        return EXIT_FAILURE;
    }

    char *message_buffer = malloc((size_t)message_length + 1);
    if (!message_buffer)
    {
        close(fd);
        return EXIT_FAILURE;
    }

    if (read_all(fd, message_buffer, (size_t)message_length) != message_length)
    {
        free(message_buffer);
        close(fd);
        return EXIT_FAILURE;
    }
    message_buffer[message_length] = '\0';

    sleep(FIFO_DELAY_SECONDS);

    time_t reader_time = time(NULL);
    char reader_timestamp[TIMESTAMP_BUFFER_SIZE];
    format_timestamp(reader_time, reader_timestamp, sizeof(reader_timestamp));

    printf("FIFO READER pid=%d current_time=%s\nRECEIVED: %s\n", (int)getpid(),
           reader_timestamp, message_buffer);

    free(message_buffer);
    close(fd);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        return runPipe();
    }

    if (argc >= 2 && strcmp(argv[1], "fw") == 0)
    {
        if (mkfifo(FIFO_PATH, FIFO_PERMISSIONS) < 0 && errno != EEXIST)
        {
            return EXIT_FAILURE;
        }
        return runFifoWrite(FIFO_PATH);
    }

    if (argc >= 2 && strcmp(argv[1], "fr") == 0)
    {
        if (mkfifo(FIFO_PATH, FIFO_PERMISSIONS) < 0 && errno != EEXIST)
        {
            return EXIT_FAILURE;
        }
        return runFifoRead(FIFO_PATH);
    }

    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s run pipe+fork\n", argv[0]);
    fprintf(stderr, "  %s fifo-write run FIFO writer\n", argv[0]);
    fprintf(stderr, "  %s fifo-read run FIFO reader\n",  argv[0]);

    return EXIT_FAILURE;
}
