#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

static int shared_memory_id = FTOK_ERROR;
static void *shared_memory_addr = NULL;

static void cleanup_resources(int signal);
static void create_shared_memory_file(void);
static void setup_shared_memory(void);
static void write_to_shared_memory(void);
static void writer_main_loop(void);

int main(void)
{
    create_shared_memory_file();
    
    setup_shared_memory();
    
    signal(SIGNAL_INTERRUPT, cleanup_resources);
    signal(SIGNAL_TERMINATE, cleanup_resources);
    
    writer_main_loop();
}

static void create_shared_memory_file(void)
{
    int file_descriptor;
    
    file_descriptor = open(SHMEM_FILE_PATH, O_CREAT | O_WRONLY | O_EXCL, 
                          SHMEM_PERMISSIONS);
    
    if (file_descriptor == FTOK_ERROR) {
        if (errno == EEXIST) {
            fprintf(stderr, "[writer]: only one process can be ran \"writer\"!\n");
        } else {
            perror("[writer]: open FAILED");
        }
        exit(EXIT_ERROR);
    }
    
    close(file_descriptor);
}

static void setup_shared_memory(void)
{
    key_t memory_key;
    
    memory_key = ftok(SHMEM_FILE_PATH, SHMEM_PROJECT_ID);
    if (memory_key == FTOK_ERROR) {
        perror("[writer]: ftok FAILED");
        exit(EXIT_ERROR);
    }
    
    shared_memory_id = shmget(memory_key, SHMEM_BUFFER_SIZE, 
                             SHMEM_CREATE_FLAGS | SHMEM_PERMISSIONS);
    if (shared_memory_id == FTOK_ERROR) {
        perror("[writer]: shmget FAILED");
        exit(EXIT_ERROR);
    }
    
    shared_memory_addr = shmat(shared_memory_id, SHMAT_AUTO, SHMEM_READ_WRITE);
    if (shared_memory_addr == SHMAT_ERROR) {
        perror("[writer]: shmat FAILED");
        exit(EXIT_ERROR);
    }
    
    memset(shared_memory_addr, 0, SHMEM_BUFFER_SIZE);
    
    printf("[writer]: ID SHM: %d\n", shared_memory_id);
}

static void write_to_shared_memory(void)
{
    char write_buffer[SHMEM_BUFFER_SIZE];
    char time_string[MAX_TIME_STR];
    time_t current_time;
    
    current_time = time(NULL);
    strcpy(time_string, ctime(&current_time));
    
    time_string[strlen(time_string) - 1] = '\0';
    
    snprintf(write_buffer, SHMEM_BUFFER_SIZE,
             "[writer]: PID: %d, Time: %s\n",
             getpid(), time_string);
    
    memcpy(shared_memory_addr, write_buffer, SHMEM_BUFFER_SIZE);
    
    printf("[writer]: wrote data\n");
    printf("[writer]: %s", write_buffer);
}

static void writer_main_loop(void)
{
    printf("[writer]: writer is working (PID: %d)\n", getpid());
    
    while (1) {
        write_to_shared_memory();
        printf("---\n");
        sleep(WRITER_SLEEP_TIME);
    }
}

static void cleanup_resources(int signal)
{
    (void)signal;
    
    if (shared_memory_addr != NULL && shared_memory_addr != SHMAT_ERROR) {
        shmdt(shared_memory_addr);
        printf("[writer]: disconnent from SHM\n");
    }
    
    if (shared_memory_id != FTOK_ERROR) {
        shmctl(shared_memory_id, IPC_RMID, NULL);
        printf("[writer]: SHM was deleted\n");
    }
    
    if (unlink(SHMEM_FILE_PATH) == FTOK_ERROR) {
        perror("[writer]: remove FAILED");
    } else {
        printf("[writer]: Key file was deleted: %s\n", SHMEM_FILE_PATH);
    }
    
    exit(EXIT_NORMAL);
}
