#ifndef COMMON_H
#define COMMON_H

#include <fcntl.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <signal.h>
#include <time.h>
#include <unistd.h>

#define SHM_SIZE 1024
#define FTOK_PROJECT_ID 1
#define SEMAPHORE_INITIAL_VALUE 1
#define SEMAPHORE_INDEX 0

#define SHMEM_FILE_PATH "/tmp/.shmem"
#define SEM_FILE_PATH "/tmp/.semshm"

#define SUCCESS 0
#define ERROR -1
#define SHM_ATTACH_FAILED ((void *)-1)

#define SEMAPHORE_LOCK_OPERATION -1
#define SEMAPHORE_UNLOCK_OPERATION 1
#define NO_FLAGS 0

#define SHM_OPEN_FLAGS (O_CREAT | O_EXCL | O_WRONLY)
#define SEM_OPEN_FLAGS (O_CREAT | O_EXCL | O_RDWR)
#define FILE_PERMISSIONS 0666

#endif // COMMON_H
