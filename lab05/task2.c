#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#define OK 0
#define SHARED_ERR 1
#define SEM_ERR 2
#define FORK_ERR 3

#define COUNT_WRITERS 5
#define COUNT_READERS 5

#define START_SEM_LEN 5
#define STOP_SEM_LEN 1

#define COUNT 3

#define SEM_WRITER_ACT 0
#define SEM_READER_ACT 1
#define SEM_WRITER_WAITING 2
#define SEM_READER_WAITING 3

#define P -1
#define Z 0
#define V 1

int *data = NULL;

struct sembuf start_write[START_SEM_LEN] = {
    {SEM_WRITER_WAITING, V, 0},
    {SEM_READER_ACT,     Z, 0},
    {SEM_WRITER_ACT,     Z, 0},
    {SEM_WRITER_ACT,     V, 0},
    {SEM_WRITER_WAITING, P, 0}
};

struct sembuf stop_write[STOP_SEM_LEN] = {
    {SEM_WRITER_ACT,     P, 0}
};

struct sembuf start_read[START_SEM_LEN] = {
    {SEM_READER_WAITING, V, 0},
    {SEM_WRITER_WAITING, Z, 0},
    {SEM_WRITER_ACT,     Z, 0},
    {SEM_READER_ACT,     V, 0},
    {SEM_READER_WAITING, P, 0}
};

struct sembuf stop_read[STOP_SEM_LEN] = {
    {SEM_READER_ACT,     P, 0}
};


void starting_write(const int id_sem) {
    if (semop(id_sem, start_write, START_SEM_LEN) == -1) {
        perror("Operation with semaphores is impossible.");
        exit(SEM_ERR);
    }
}

void stoping_writer(const int id_sem) {
    if (semop(id_sem, stop_write, STOP_SEM_LEN) == -1) {
        perror("Operation with semaphores is impossible.");
        exit(SEM_ERR);
    }
}

void run_writer(const int id_sem, const int id) {
    for (int i = 0; i < COUNT; ++i) {
        sleep(rand() % 4);
        starting_write(id_sem);
        (*data)++;
        printf("Writer %d >>>> %d\n" , id, *data );
        stoping_writer(id_sem);
    }
}


void execute_writer(const int id_sem) {
    pid_t pid;

    for (int i = 0; i < COUNT_WRITERS; ++i) {
        pid = fork();
        if (pid == -1) {
            puts("Can not fork!");
            exit(FORK_ERR);
        } else if (pid == 0) {
            run_writer(id_sem, i);
            exit(OK);
        }
    }
}


void starting_read(const int id_sem) {
    if (semop(id_sem, start_read, START_SEM_LEN) == -1) {
        perror("Operation with semaphores is impossible.");
        exit(SEM_ERR);
    }
}


void stoping_read(const int id_sem) {
    if (semop(id_sem, stop_read, STOP_SEM_LEN) == -1) {
        perror("Operation with semaphores is impossible.");
        exit(SEM_ERR);
    }
}

void run_read(const int id_sem, const int id) {
    for (int i = 0; i < COUNT; ++i) {
        sleep(rand() % 4);
        starting_read(id_sem);
        printf("Reader %d <<<< %d\n" , id, *data );
        stoping_read(id_sem);
    }
}


void execute_reader(const int id_sem) {
    pid_t pid;

    for (int i = 0; i < COUNT_WRITERS; ++i) {
        pid = fork();
        if (pid == -1) {
            puts("Can not fork!");
            exit(FORK_ERR);
        } else if (pid == 0) {
            run_read(id_sem, i);
            exit(OK);
        }
    }
}

void wait_processes() {
    int status;
    pid_t data_pr;

    for (size_t i = 0; i < COUNT_READERS + COUNT_WRITERS; ++i) {
        data_pr = wait(&status);

        fprintf(stdout, "Child process has finished, pid = %d, status = %d\n", 
                data_pr, status);

        if (WIFEXITED(status)) {
            fprintf(stdout, "Child process (%d) finished, code = %d.\n\n", 
                    data_pr, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stdout, "Child process (%d) finished, code = %d.\n\n", 
                    data_pr, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            fprintf(stdout, "Child process (%d) finished, code = %d.\n\n", 
                    data_pr, WSTOPSIG(status));
        }   
    }
}

int main() {
    int id_shared, id_sem, status;
    pid_t pid = getpid();

    fprintf(stdout, "Parent process pid = %d\n", pid);
    id_shared = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO);
    if (id_shared == -1) {
        perror("Unable to create a shared area.");
        exit(SHARED_ERR);
    }
    data = shmat(id_shared, 0, 0);

    id_sem = semget(IPC_PRIVATE, 5, IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO);
    if (id_sem == -1) {
        perror("Unable to create a semaphore.");
        exit(SEM_ERR);
    }

    if (semctl(id_sem, SEM_WRITER_ACT, SETVAL, 0) == -1 ||
        semctl(id_sem, SEM_READER_ACT, SETVAL, 0) == -1 ||
        semctl(id_sem, SEM_WRITER_WAITING, SETVAL, 0) == -1 ||
        semctl(id_sem, SEM_READER_WAITING, SETVAL, 0) == -1)
    {
        perror("Can not set values semaphors.");
        exit(SEM_ERR);
    }

    execute_writer(id_sem);
    execute_reader(id_sem);
    wait_processes();

    if (shmdt(data) == -1) {
        perror("Can not detach shared memory");
        exit(SHARED_ERR);
    }

    if (pid != 0) {
        if (shmctl(id_shared, IPC_RMID, NULL) == -1) {
            perror("Can not free memory!");
            exit(SHARED_ERR);
        }
    }

    exit(OK);
}