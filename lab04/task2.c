#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define FORK_ERROR 1
#define OK 0


int main() {
    pid_t first_ch, second_ch;
    int status, data;
    
    first_ch = fork();

    if (first_ch == - 1) {
        perror("Can't fork.\n ");
        return FORK_ERROR;
    } else if (first_ch == 0) {
        sleep(2);
        fprintf(stdout, "\n1st child process:\tpid = %d\tppid = %d\tgroup id = %d\n", 
                getpid(), getppid(), getpgrp());

        return OK;
    } else {
        data = wait(&status);

        if (WIFEXITED(status)) {
            fprintf(stdout, "Child process (%d) finished with code %d\n", 
                    data, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stdout, "Child process (%d) finished from signal with code %d\n",
                    data, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            fprintf(stdout, "Child process (%d) finished from signal with code %d\n",
                    data, WSTOPSIG(status));
        }

        second_ch = fork();

        if (second_ch == - 1) {
            perror("Can't fork.\n ");
            return FORK_ERROR;
        } else if (second_ch == 0) {
            sleep(2);
            fprintf(stdout, "\n2nd child process:\tpid = %d\tppid = %d\tgroup id = %d\n",
                    getpid(), getppid(), getpgrp());

            return OK;
        }

        data = wait(&status);

        if (WIFEXITED(status)) {
            fprintf(stdout, "Child process (%d) finished with code %d\n", 
                    data, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stdout, "Child process (%d) finished from signal with code %d\n",
                    data, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            fprintf(stdout, "Child process %d finished from signal with code %d\n",
                    data, WSTOPSIG(status));
        }

        fprintf(stdout, "\nParent process:\tpid = %d\tchild proc. id = %d, %d\tgroup id = %d\n",
                getpid(), first_ch , second_ch, getpgrp());

        return OK;
    }

    return OK;
}