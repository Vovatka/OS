#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define READERS_COUNT 10
#define BUFFER_SIZE 10
#define WRITER_SLEEP_TIME 1
#define READER_SLEEP 50000

static int shared_buffer[BUFFER_SIZE];
static int message_counter = 0;
static int index = 0;

pthread_rwlock_t buffer_rwlock;

void *writer_thread(void *arg)
{
    (void)arg;

    while (1)
    {
        pthread_rwlock_wrlock(&buffer_rwlock);
        shared_buffer[index] = message_counter++;
        index = (index + 1) % BUFFER_SIZE;
        pthread_rwlock_unlock(&buffer_rwlock);
        sleep(WRITER_SLEEP_TIME);
    }

    return NULL;
}

void *reader_thread(void *arg)
{
    (void)arg;
    pthread_t thread_id = pthread_self();

    while (1)
    {
        pthread_rwlock_rdlock(&buffer_rwlock);
        printf("[READER] TID=%lu: ", (unsigned long)thread_id);

        for (int i=0; i < BUFFER_SIZE; i++)
        {
            printf("%d ", shared_buffer[i]);
        }
        printf("\n");

        pthread_rwlock_unlock(&buffer_rwlock);
        usleep(READER_SLEEP);
    }

    return NULL;
}

int main(void)
{
    pthread_t writer_tid;
    pthread_t reader_tids[READERS_COUNT];
    int thread_creation_result;

    int rwlock_init_result = pthread_rwlock_init(&buffer_rwlock, NULL);
    if (rwlock_init_result != 0)
    {
        perror("Error initializing rwlock");
        return EXIT_FAILURE;
    }

    thread_creation_result =
        pthread_create(&writer_tid, NULL, writer_thread, NULL);
    if (thread_creation_result != 0)
    {
        perror("Error creating writer thread");
        pthread_rwlock_destroy(&buffer_rwlock);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < READERS_COUNT; i++)
    {
        thread_creation_result =
            pthread_create(&reader_tids[i], NULL, reader_thread, NULL);
        if (thread_creation_result != 0)
        {
            fprintf(stderr, "Error creating reader thread %d: %d\n", i,
                    thread_creation_result);
            pthread_rwlock_destroy(&buffer_rwlock);
            return EXIT_FAILURE;
        }
    }

    pthread_join(writer_tid, NULL);

    pthread_rwlock_destroy(&buffer_rwlock);

    return EXIT_SUCCESS;
}
