#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define READERS_COUNT 10
#define BUFFER_SIZE 10
#define WRITER_SLEEP_TIME 1
#define READER_SLEEP_US 50000

static int shared_buffer[BUFFER_SIZE] = {0};
static int message_counter = 0;
static int index = 0;

static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t reader_finished_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t writer_finished_cond = PTHREAD_COND_INITIALIZER;

static int active_readers = 0;
static bool writer_active = false;
static bool writer_waiting = false;

void *writer_thread(void *arg)
{
    (void)arg;

    while (1)
    {
        pthread_mutex_lock(&buffer_mutex);

        while (active_readers > 0 || writer_active)
        {
            writer_waiting = true;
            pthread_cond_wait(&reader_finished_cond, &buffer_mutex);
        }

        writer_active = true;
        writer_waiting = false;
        shared_buffer[index] = message_counter++;
        index = (index + 1) % BUFFER_SIZE;
        writer_active = false;

        pthread_cond_broadcast(&writer_finished_cond);
        pthread_mutex_unlock(&buffer_mutex);

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
        pthread_mutex_lock(&buffer_mutex);

        while (writer_active || writer_waiting)
        {
            pthread_cond_wait(&writer_finished_cond, &buffer_mutex);
        }

        active_readers++;

        pthread_mutex_unlock(&buffer_mutex);
        printf("[READER] TID=%lu: ", (unsigned long)thread_id);

        for (int i = 0; i < BUFFER_SIZE; i++)
        {
            printf("%d ", shared_buffer[i]);
        }
        printf("\n");

        pthread_mutex_lock(&buffer_mutex);
        active_readers--;

        if (active_readers == 0)
        {
            pthread_cond_signal(&reader_finished_cond);
        }

        pthread_mutex_unlock(&buffer_mutex);

        usleep(READER_SLEEP_US);
    }

    return NULL;
}

int main(void)
{
    pthread_t writer_tid;
    pthread_t reader_tids[READERS_COUNT];

    if (pthread_create(&writer_tid, NULL, writer_thread, NULL) != 0)
    {
        perror("Failed to create writer thread");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < READERS_COUNT; i++)
    {
        if (pthread_create(&reader_tids[i], NULL, reader_thread, NULL) != 0)
        {
            fprintf(stderr, "Failed to create reader thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    pthread_join(writer_tid, NULL);

    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&reader_finished_cond);
    pthread_cond_destroy(&writer_finished_cond);

    return EXIT_SUCCESS;
}
