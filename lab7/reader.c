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

static void *shared_memory_addr = NULL;

static void cleanup_resources(int signal);
static void attach_to_shared_memory(void);
static void read_shared_memory_loop(void);

int main(void)
{
    signal(SIGNAL_INTERRUPT, cleanup_resources);
    signal(SIGNAL_TERMINATE, cleanup_resources);
    
    attach_to_shared_memory();
    
    read_shared_memory_loop();
}

static void cleanup_resources(int signal)
{
    (void)signal;    
    if (shared_memory_addr != NULL && shared_memory_addr != SHMAT_ERROR) {
        shmdt(shared_memory_addr);
    }
    
    exit(EXIT_NORMAL);
}

static void attach_to_shared_memory(void)
{
    key_t memory_key;
    
    memory_key = ftok(SHMEM_FILE_PATH, SHMEM_PROJECT_ID);
    if (memory_key == FTOK_ERROR) {
        if (errno == ENOENT) {
            fprintf(stderr, "[reader]: run \"writer\" first!\n");
        } else {
            perror("[reader]: ftok FAILED");
        }
        exit(EXIT_ERROR);
    }
    
    int memory_id = shmget(memory_key, SHMEM_BUFFER_SIZE, SHMEM_GET_FLAGS);
    if (memory_id == FTOK_ERROR) {
        perror("[reader]: shmget FAILED");
        exit(EXIT_ERROR);
    }
    
    shared_memory_addr = shmat(memory_id, SHMAT_AUTO, SHMEM_READ_ONLY);
    if (shared_memory_addr == SHMAT_ERROR) {
        perror("[reader]: shmat FAILED");
        exit(EXIT_ERROR);
    }
    
    printf("[reader]: connect to SHM SUCCESS\n");
}

static void read_shared_memory_loop(void)
{
    char read_buffer[SHMEM_BUFFER_SIZE];
    
    printf("[reader]: reading (PID: %d)\n", getpid());
    
    while (1) {
        time_t current_time = time(NULL);
        printf("[reader]: PID: %d, Time: %s", 
               getpid(), ctime(&current_time));
        
        memcpy(read_buffer, shared_memory_addr, SHMEM_BUFFER_SIZE);
        printf("[reader]: got string: \"%s\"", read_buffer);
        
        printf("\n-------------------\n");
        
        sleep(READER_SLEEP_TIME);
    }
}
