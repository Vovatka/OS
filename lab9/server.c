#include "common.h"

static key_t shared_memory_key;
static key_t semaphore_key;
static int shared_memory_id = 0;
static void *shared_memory_address = NULL;
static int semaphore_id = 0;

static void cleanup_existing_resources(void)
{
    unlink(SHMEM_FILE_PATH);
    unlink(SEM_FILE_PATH);

    key_t temp_key;

    temp_key = ftok(SHMEM_FILE_PATH, FTOK_PROJECT_ID);
    if (temp_key != (key_t)ERROR)
    {
        int temp_shm_id = shmget(temp_key, SHM_SIZE, 0666);
        if (temp_shm_id != ERROR)
        {
            shmctl(temp_shm_id, IPC_RMID, NULL);
        }
    }

    temp_key = ftok(SEM_FILE_PATH, FTOK_PROJECT_ID);
    if (temp_key != (key_t)ERROR)
    {
        int temp_sem_id = semget(temp_key, 1, 0666);
        if (temp_sem_id != ERROR)
        {
            semctl(temp_sem_id, SEMAPHORE_INDEX, IPC_RMID);
        }
    }
}

static void cleanup_resources(int signal)
{
    (void)signal;

    if (shared_memory_address && shared_memory_address != SHM_ATTACH_FAILED)
    {
        shmdt(shared_memory_address);
        printf("Shared memory detached\n");
    }

    if (shared_memory_id)
    {
        shmctl(shared_memory_id, IPC_RMID, NULL);
        printf("Shared memory segment removed\n");
    }

    if (semaphore_id)
    {
        semctl(semaphore_id, SEMAPHORE_INDEX, IPC_RMID);
        printf("Semaphore removed\n");
    }

    if (unlink(SHMEM_FILE_PATH) == SUCCESS)
    {
        printf("File %s removed\n", SHMEM_FILE_PATH);
    }

    if (unlink(SEM_FILE_PATH) == SUCCESS)
    {
        printf("File %s removed\n", SEM_FILE_PATH);
    }

    exit(SUCCESS);
}

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

static int initialize_resources(void)
{
    int file_descriptor;

    cleanup_existing_resources();

    file_descriptor = open(SHMEM_FILE_PATH, SHM_OPEN_FLAGS, FILE_PERMISSIONS);
    if (file_descriptor == ERROR)
    {
        fprintf(stderr, "Failed to create %s: %s\n", SHMEM_FILE_PATH,
                strerror(errno));
        return ERROR;
    }
    close(file_descriptor);

    file_descriptor = open(SEM_FILE_PATH, SEM_OPEN_FLAGS, FILE_PERMISSIONS);
    if (file_descriptor == ERROR)
    {
        fprintf(stderr, "Failed to create %s: %s\n", SEM_FILE_PATH,
                strerror(errno));
        unlink(SHMEM_FILE_PATH);
        return ERROR;
    }
    close(file_descriptor);

    shared_memory_key = ftok(SHMEM_FILE_PATH, FTOK_PROJECT_ID);
    if (shared_memory_key == (key_t)ERROR)
    {
        fprintf(stderr, "Failed to get key for %s: %s\n", SHMEM_FILE_PATH,
                strerror(errno));
        return ERROR;
    }

    semaphore_key = ftok(SEM_FILE_PATH, FTOK_PROJECT_ID);
    if (semaphore_key == (key_t)ERROR)
    {
        fprintf(stderr, "Failed to get key for %s: %s\n", SEM_FILE_PATH,
                strerror(errno));
        return ERROR;
    }

    semaphore_id =
        semget(semaphore_key, 1, IPC_CREAT | IPC_EXCL | FILE_PERMISSIONS);
    if (semaphore_id == ERROR)
    {
        fprintf(stderr, "Failed to create semaphore");
        return ERROR;
    }

    shared_memory_id = shmget(shared_memory_key, SHM_SIZE,
                              IPC_CREAT | IPC_EXCL | FILE_PERMISSIONS);
    if (shared_memory_id == ERROR)
    {
        perror("Failed to get shared memory\n");
        return ERROR;
    }

    shared_memory_address = shmat(shared_memory_id, NULL, NO_FLAGS);
    if (shared_memory_address == SHM_ATTACH_FAILED)
    {
        perror("Failed to attach shared memory\n");
        return ERROR;
    }

    semctl(semaphore_id, SEMAPHORE_INDEX, SETVAL, SEMAPHORE_INITIAL_VALUE);
    memset(shared_memory_address, 0, SHM_SIZE);

    printf("Shared memory file: %s\n", SHMEM_FILE_PATH);
    printf("Semaphore file:     %s\n", SEM_FILE_PATH);

    return SUCCESS;
}

int main(void)
{
    signal(SIGTERM, cleanup_resources);
    signal(SIGINT, cleanup_resources);

    if (initialize_resources() == ERROR)
    {
        return EXIT_FAILURE;
    }

    const int WRITE_INTERVAL_SECONDS = 3;

    printf("\nServer started. PID: %d\n", getpid());

    while (true)
    {
        lock_semaphore(semaphore_id);

        time_t current_time = time(NULL);
        pid_t process_id = getpid();

        snprintf((char *)shared_memory_address, SHM_SIZE, "[%d] time: %ld",
                 process_id, current_time);

        printf("Written to shared memory: %s\n", (char *)shared_memory_address);

        unlock_semaphore(semaphore_id);

        sleep(WRITE_INTERVAL_SECONDS);
    }

    return SUCCESS;
}
