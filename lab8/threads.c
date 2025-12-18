#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define THREAD_COUNT       10
#define STRSIZE           1024
#define WRITER_ITERATIONS  5
#define READER_SLEEP_SEC   1
#define WRITER_SLEEP_SEC   1
#define TERMINATE_STRING   "done"

char str[STRSIZE];
pthread_mutex_t str_mx = PTHREAD_MUTEX_INITIALIZER;

void *reader_func(void *args)
{
    pthread_t *tid_ptr = (pthread_t *)args;
    pthread_t tid = *tid_ptr;

    while (1)
    {
        pthread_mutex_lock(&str_mx);

        if (strcmp(str, TERMINATE_STRING) == 0)
        {
            pthread_mutex_unlock(&str_mx);
            break;
        }

        printf("tid: %lu: '%s'\n", (unsigned long)tid, str);
        pthread_mutex_unlock(&str_mx);
        sleep(READER_SLEEP_SEC);
    }
    return NULL;
}

void *writer_func(void *args)
{
    int *counter = (int *)args;

    while (1)
    {
        pthread_mutex_lock(&str_mx);

        (*counter)++;
        sprintf(str, "[worker]: num=%d", *counter);

        if (*counter == WRITER_ITERATIONS)
        {
            strcpy(str, TERMINATE_STRING);
            pthread_mutex_unlock(&str_mx);
            break;
        }

        pthread_mutex_unlock(&str_mx);
        sleep(WRITER_SLEEP_SEC);
    }
    return NULL;
}

int main(void)
{
    int counter = 0;
    pthread_attr_t thread_attrs;
    pthread_t thread_pool[THREAD_COUNT];
    pthread_t thread_writer;

    if (pthread_attr_init(&thread_attrs) != 0)
    {
        perror("pthread_attr_init FAILED");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread_writer, &thread_attrs, writer_func, &counter) != 0)
    {
        perror("pthread_create for writer FAILED");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        if (pthread_create(&thread_pool[i], &thread_attrs, reader_func, &thread_pool[i]) != 0)
        {
            perror("pthread_create for reader FAILED");
            exit(EXIT_FAILURE);
        }
    }

    pthread_attr_destroy(&thread_attrs);

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        if (pthread_join(thread_pool[i], NULL) != 0)
        {
            perror("pthread_join for reader FAILED");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_join(thread_writer, NULL) != 0)
    {
        perror("pthread_join for writer FAILED");
        exit(EXIT_FAILURE);
    }

    return 0;
}
