#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{

    int status_code1 = 0;
    int status_code2 = 0;

    int fd[2];
    int fd1[2];

    pid_t pid1;
    pid_t pid2;

    char *first[2];
    char *second[3];
    first[0] = "ls";
    first[1] = NULL;
    second[0] = "wc";
    second[1] = "-l";
    second[2] = NULL;

    if (pipe(fd) == -1)
    {
        printf("Error creating Pipe 1\n");
    }

    pid1 = fork();

    if (pid1 == 0)
    {
        pid2 = fork();

        if (pid2 == 0)
        {
            close(1);
            dup(fd[1]);
            close(fd[1]);
            close(fd[0]);
            execvp(first[0], first);
            printf("First commit\n");
            exit(1);
        } else {
            waitpid(pid2, &status_code2, 0);
            close(0);
            dup(fd[0]);
            close(fd[0]);
            close(fd[1]);
            execvp(second[0], second);
            printf("Second commit\n");
            exit(1);
        }
    } else {
        close(fd[1]);
        close(fd[0]);
        int result = waitpid(pid1, &status_code1, 0);
    }

    return 0;
}