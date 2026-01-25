#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 64
#define INITIAL_VALUE 0
#define SEMAPHORE_INITIAL_VALUE 1
#define SLEEP_TIME_SECONDS 1
#define THREAD_SUCCESS 0

static char shared_buffer[BUFFER_SIZE];
static sem_t buffer_semaphore;

void *data_writer(void *arg)
{
    (void)arg;
    int counter = INITIAL_VALUE;

    while (1)
    {
        snprintf(shared_buffer, sizeof(shared_buffer), "%d", counter++);
        sem_post(&buffer_semaphore);
        sleep(SLEEP_TIME_SECONDS);
    }

    pthread_exit(NULL);
}

void *data_reader(void *arg)
{
    (void)arg;

    while (1)
    {
        sem_wait(&buffer_semaphore);
        printf("[READER] TID=%lu buffer=%s\n", (unsigned long)pthread_self(),
               shared_buffer);
    }

    pthread_exit(NULL);
}

int main(void)
{
    pthread_t writer_thread, reader_thread;
    int operation_result;

    operation_result = sem_init(&buffer_semaphore, 0, SEMAPHORE_INITIAL_VALUE);
    if (operation_result != THREAD_SUCCESS)
    {
        perror("Error initializing semaphore");
        return EXIT_FAILURE;
    }

    strcpy(shared_buffer, "0");

    operation_result = pthread_create(&writer_thread, NULL, data_writer, NULL);
    if (operation_result != THREAD_SUCCESS)
    {
        perror("Error creating writer thread");
        sem_destroy(&buffer_semaphore);
        return EXIT_FAILURE;
    }

    operation_result = pthread_create(&reader_thread, NULL, data_reader, NULL);
    if (operation_result != THREAD_SUCCESS)
    {
        perror("Error creating reader thread");
        sem_destroy(&buffer_semaphore);
        return EXIT_FAILURE;
    }

    pthread_join(writer_thread, NULL);
    pthread_join(reader_thread, NULL);

    sem_destroy(&buffer_semaphore);

    return EXIT_SUCCESS;
}
