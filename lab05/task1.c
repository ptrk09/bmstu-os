#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define OK 0
#define SHARED_ERR 1
#define SEM_ERR 2
#define FORK_ERR 3

#define COUNT_PRODUCER 5
#define COUNT_CONSUMERS 5
#define BUFFER_LEN 10
#define SEMBUF_LEN 2
#define COUNT_SYM 4

#define BIN_SEM 0
#define EMPT_SEM 1
#define FULL_SEM 2

#define P -1
#define V 1


struct sembuf producer_act[SEMBUF_LEN] = { 
    {EMPT_SEM, P, 0}, 
    {BIN_SEM,  P, 0} 
};

struct sembuf consumer_act[SEMBUF_LEN] = { 
    {FULL_SEM, P, 0}, 
    {BIN_SEM,  P, 0} 
};

struct sembuf producer_deact[SEMBUF_LEN] =  { 
    {BIN_SEM,  V, 0}, 
    {FULL_SEM, V, 0} 
};

struct sembuf consumer_deact[SEMBUF_LEN] =  { 
    {BIN_SEM,  V, 0}, 
    {EMPT_SEM, V, 0} 
};

char *buffer, *cur_pos_write, *cur_pos_read, *sym;


void producer(const int semid, const int consumerNum) {
    for (char i = 0; i < COUNT_SYM; ++i) {
        sleep(rand() % 2);

        if (semop(semid, producer_act, 2) == -1) {
            perror("Operation with semaphores is impossible.");
            exit(SEM_ERR);
        }

        buffer[(*cur_pos_write) % BUFFER_LEN] = (*sym)++;
        fprintf(stdout, "Producer %d >>>>> %c\n",
                consumerNum, buffer[((*cur_pos_write)++) % BUFFER_LEN]);

        if (semop(semid, producer_deact, 2) == -1) {
            perror("Operation with semaphores is impossible.");
            exit(SEM_ERR);
        }
    }
}


void consumer(const int semid, const int consumerNum) {
    for (int i = 0; i < COUNT_SYM; i++) {
        sleep(rand() % 5);

        if (semop(semid, consumer_act, 2) == -1) {
            perror("Operation with semaphores is impossible");
            exit(SEM_ERR);
        }

        fprintf(stdout, "Consumer %d <<<<< %c\n", 
                consumerNum, buffer[((*cur_pos_read)++) % BUFFER_LEN]);

        if (semop(semid, consumer_deact, 2) == -1) {
            perror("Operation with semaphores is impossible");
            exit(SEM_ERR);
        }
    }
}


void execute_consumer(const int id_sem) {
    pid_t pid;

    for (int i = 0; i < COUNT_CONSUMERS; ++i) {
        pid = fork();
        if (pid == -1) {
            puts("Can not fork!");
            exit(FORK_ERR);
        } else if (pid == 0) {
            consumer(id_sem, i + 1);
            exit(OK);
        }
    }
}


void execute_producer(const int id_sem) {
    pid_t pid;

    for (int i = 0; i < COUNT_PRODUCER; ++i) {
        pid = fork();
        if (pid == -1) {
            puts("Can not fork!");
            exit(FORK_ERR);
        } else if (pid == 0) {
            producer(id_sem, i + 1);
            exit(OK);
        }
    }
}

void wait_processes() {
    int status;
    pid_t data_pr;

    for (size_t i = 0; i < COUNT_PRODUCER + COUNT_CONSUMERS; ++i) {
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
    int id_shared, id_sem;
    pid_t pid = getpid();

    fprintf(stdout, "Parent process pid = %d\n", pid);
    id_shared = shmget(IPC_PRIVATE, (BUFFER_LEN + 3) * sizeof(char), 
                       IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO);

    if (id_shared == -1) {
        perror("Unable to create a shared area.");
        exit(SHARED_ERR);
    }

    buffer = shmat(id_shared, 0, 0);
    if ((*buffer) == -1) {
        perror("Can not attach memory");
        exit(SHARED_ERR);
    }

    cur_pos_write = buffer;
    cur_pos_read = buffer + sizeof(char);
    sym = buffer + (sizeof(char) * 2);
    buffer = buffer + (sizeof(char) * 3);

    *sym = 'A';
    *cur_pos_write = 0;
    *cur_pos_read = 0;
    
    id_sem = semget(IPC_PRIVATE, 3, IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO);
    if (id_sem == -1) {
        perror("Unable to create a semaphore.");
        exit(SEM_ERR);
    }

    if (semctl(id_sem, BIN_SEM, SETVAL, 1) == -1 || 
        semctl(id_sem, EMPT_SEM, SETVAL, BUFFER_LEN) == -1 ||
        semctl(id_sem, FULL_SEM, SETVAL, 0) == -1) 
    {
        perror("Can not set values semaphors.");
        exit(SEM_ERR);
    }

    execute_producer(id_sem);
    execute_consumer(id_sem);
    wait_processes();

    fprintf(stdout, "%d\n", *cur_pos_write);
    if (shmdt(cur_pos_write) == -1) {
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
