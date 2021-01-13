#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define LEN_STR 50

#define FORK_ERROR 1
#define PIPE_ERROR 2
#define OK 0


int main() {
    pid_t first_ch, second_ch;
    int fd_array[2];
    int status, data;
    char str_w[LEN_STR];

    if (pipe(fd_array) == -1) {
        perror("Couldn't pipe");
        return PIPE_ERROR;
    }
    
    first_ch = fork();

    if (first_ch == - 1) {
        perror("Can't fork.\n ");
        return FORK_ERROR;
    } else if (first_ch == 0) {
        close(fd_array[0]);
        if (!write(fd_array[1], "Child msg 1\n", 12)) {
            perror("Can't write\n");
            return PIPE_ERROR;
        }

        return OK;
    } else {
        data = wait(&status);

        if (WIFEXITED(status)) {
            fprintf(stdout, "Child process (%d) finished, code = %d\n", 
                    data, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stdout, "Child process (%d) finished from signal, code = %d\n",
                    data, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            fprintf(stdout, "Child process (%d) finished from signal, code = %d\n",
                    data, WSTOPSIG(status));
        }

        second_ch = fork();

        if (second_ch == - 1) {
            perror("Can't fork.\n ");
            return FORK_ERROR;
        } else if (second_ch == 0) {
            close(fd_array[0]);
            if (!write(fd_array[1], "Child msg 2\n", 12)) {
                perror("Can't write\n");
                return PIPE_ERROR;
            }

            return OK;
        }

        data = wait(&status);

        if (WIFEXITED(status)) {
            fprintf(stdout, "\nChild process (%d) finished, code = %d\n", 
                    data, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stdout, "Child process (%d) finished from signal, code = %d\n",
                    data, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            fprintf(stdout, "Child process (%d) finished from signal, code = %d\n",
                    data, WSTOPSIG(status));
        }

        close(fd_array[1]);
        if (read(fd_array[0], str_w, LEN_STR) < 0) {
            perror("Can't read\n");
            return PIPE_ERROR;
        }

        fprintf(stdout, "\n%s", str_w);

        return OK;
    }
    
    return OK;
}