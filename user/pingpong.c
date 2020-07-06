#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int parent_fd[2], child_fd[2];
    int pid;
    char buf[2];

    pipe(parent_fd);
    pipe(child_fd);
    if(fork() == 0){
        // child process.
        close(parent_fd[1]);
        close(child_fd[0]);

        pid = getpid();
        read(parent_fd[0], buf, 1);
        if(buf[0] == 'i'){
            printf("%d: received ping\n", pid);
        }
        write(child_fd[1], "o", 1);

        close(parent_fd[0]);
        close(child_fd[1]);
    }else{
        // parent process.
        close(parent_fd[0]);
        close(child_fd[1]);

        pid = getpid();
        write(parent_fd[1], "i", 1);
        read(child_fd[0], buf, 1);
        if(buf[0] == 'o'){
            printf("%d: received pong\n", pid);
        }

        close(parent_fd[1]);
        close(child_fd[0]);
    }
    exit();
}