#pragma once

#include <sys/ipc.h>

#define SHMEM_FILE_PATH    "/tmp/.shmem"
#define SHMEM_BUFFER_SIZE  512
#define SHMEM_PROJECT_ID   1
#define SHMEM_PERMISSIONS  0660

#define FTOK_SUCCESS       0
#define FTOK_ERROR         -1

#define SHMAT_AUTO         NULL
#define SHMAT_ERROR        ((void *)-1)

#define SHMEM_CREATE_FLAGS (IPC_CREAT | IPC_EXCL)
#define SHMEM_GET_FLAGS    0
#define SHMEM_READ_ONLY    SHM_RDONLY
#define SHMEM_READ_WRITE   0

#define SIGNAL_TERMINATE   SIGTERM
#define SIGNAL_INTERRUPT   SIGINT

#define EXIT_NORMAL        EXIT_SUCCESS
#define EXIT_ERROR         EXIT_FAILURE

#define READER_SLEEP_TIME  1
#define WRITER_SLEEP_TIME  3

#define MAX_TIME_STR       26
#define PID_STR_SIZE       12
#define PREFIX_SIZE        64
