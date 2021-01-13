#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define FORK_ERROR 1
#define OK 0


int main() {
    pid_t first_ch, second_ch;
    
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

        fprintf(stdout, "Parent process:\tpid = %d\tchild proc. id = %d, %d\tgroup id = %d\n",
                getpid(), first_ch , second_ch, getpgrp());

        return OK;
    }

    return OK;
}