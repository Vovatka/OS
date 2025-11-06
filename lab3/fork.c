#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHILD_EXIT_CODE 52
#define SLEEP_TIME 5

void handle_sigint()
{
    const char *message = "SIGINT received\n";
    write(STDOUT_FILENO, message, strlen(message));
}

void handle_sigterm()
{
    const char *message = "SIGTERM received\n";
    write(STDOUT_FILENO, message, strlen(message));
}

void handle_process_exit()
{
    printf("[atexit] Process PID=%d exiting\n", getpid());
}

int main(int argc, char** argv)
{
    atexit(handle_process_exit);
    signal(SIGINT, handle_sigint);

    struct sigaction signal_action;
    memset(&signal_action, 0, sizeof(signal_action));
    signal_action.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &signal_action, NULL);

    fflush(stdout);

    pid_t child_process_id = fork();
    
    if (child_process_id < 0)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (child_process_id == 0)
    {
        printf("Child: PID=%d, PPID=%d\n", getpid(), getppid());
        sleep(SLEEP_TIME);
        exit(CHILD_EXIT_CODE);
    }
    else
    {
        printf("Parent: PID=%d, PPID=%d, child PID=%d\n", 
               getpid(), getppid(), child_process_id);

        int child_status;
        if (waitpid(child_process_id, &child_status, 0) == -1)
        {
            perror("waitpid failed");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(child_status))
        {
            printf("Child exited with code %d\n", WEXITSTATUS(child_status));
        }
        else if (WIFSIGNALED(child_status))
        {
            printf("Child terminated by signal %d\n", WTERMSIG(child_status));
        }
    }

    return EXIT_SUCCESS;
}
