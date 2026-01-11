#include "common.h"

static key_t shared_memory_key;
static key_t semaphore_key;
static int shared_memory_id = 0;
static void *shared_memory_address = NULL;
static int semaphore_id = 0;

static void lock_semaphore(int sem_id)
{
    struct sembuf semaphore_operation = {.sem_num = SEMAPHORE_INDEX,
                                         .sem_op = SEMAPHORE_LOCK_OPERATION,
                                         .sem_flg = NO_FLAGS};
    semop(sem_id, &semaphore_operation, 1);
}

static void unlock_semaphore(int sem_id)
{
    struct sembuf semaphore_operation = {.sem_num = SEMAPHORE_INDEX,
                                         .sem_op = SEMAPHORE_UNLOCK_OPERATION,
                                         .sem_flg = NO_FLAGS};
    semop(sem_id, &semaphore_operation, 1);
}

static void cleanup_resources(int signal)
{
    (void)signal;

    if (shared_memory_address && shared_memory_address != SHM_ATTACH_FAILED)
    {
        shmdt(shared_memory_address);
        printf("Shared memory detached\n");
    }

    exit(SUCCESS);
}

static int connect_to_resources(void)
{
    shared_memory_key = ftok(SHMEM_FILE_PATH, FTOK_PROJECT_ID);
    if (shared_memory_key == (key_t)ERROR)
    {
        fprintf(stderr, "Failed to get key for %s: %s\n", SHMEM_FILE_PATH,
                strerror(errno));
        fprintf(stderr, "Start the server first\n");
        return ERROR;
    }

    semaphore_key = ftok(SEM_FILE_PATH, FTOK_PROJECT_ID);
    if (semaphore_key == (key_t)ERROR)
    {
        fprintf(stderr, "Failed to get key for %s: %s\n", SEM_FILE_PATH,
                strerror(errno));
        fprintf(stderr, "Start the server first\n");
        return ERROR;
    }

    semaphore_id = semget(semaphore_key, 1, NO_FLAGS);
    if (semaphore_id == ERROR)
    {
        fprintf(stderr, "Failed to connect to semaphore: %s\n",
                strerror(errno));
        fprintf(stderr, "Start the producer first\n");
        return ERROR;
    }

    shared_memory_id = shmget(shared_memory_key, SHM_SIZE, NO_FLAGS);
    if (shared_memory_id == ERROR)
    {
        fprintf(stderr, "Failed to connect to shared memory: %s\n",
                strerror(errno));
        fprintf(stderr, "Start the server first\n");
        return ERROR;
    }

    shared_memory_address = shmat(shared_memory_id, NULL, SHM_RDONLY);
    if (shared_memory_address == SHM_ATTACH_FAILED)
    {
        fprintf(stderr, "Failed to attach shared memory: %s\n",
                strerror(errno));
        return ERROR;
    }

    return SUCCESS;
}

int main(void)
{
    signal(SIGTERM, cleanup_resources);
    signal(SIGINT, cleanup_resources);

    if (connect_to_resources() == ERROR)
    {
        return EXIT_FAILURE;
    }

    const int READ_INTERVAL_SECONDS = 1;

    while (true)
    {
        lock_semaphore(semaphore_id);

        time_t current_time = time(NULL);
        pid_t process_id = getpid();

        printf("Received from server: %s\n", (char *)shared_memory_address);
        printf("Current client info   : [%d] time: %ld\n", process_id,
               current_time);
        printf("\n");

        unlock_semaphore(semaphore_id);

        sleep(READ_INTERVAL_SECONDS);
    }

    return SUCCESS;
}
