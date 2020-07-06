#include "kernel/types.h"
#include "user/user.h"

void redirect(int pipe_right[])
{
    close(0);
    close(pipe_right[1]);
    dup(pipe_right[0]);
    close(pipe_right[0]);
}

void
sieve()
{
    int p, n, pipe_right[2];

loop:
    if((read(0, &n, sizeof(int))) <= 0 || n <= 0){
        close(0);
        exit();
    }
    printf("prime %d\n", n);

    pipe(pipe_right);
    // fork right neighbor.
    if(fork() == 0){
        // redirec pipe_right[0] to standard input file descriptor 0.
        redirect(pipe_right);
        goto loop;
    }

    close(pipe_right[0]);
    while((read(0, &p, sizeof(int))) > 0 && p > 0){
        if(p % n){
            write(pipe_right[1], &p, sizeof(int));
        }
    }
    close(0);
    close(pipe_right[1]);
    wait();
    exit();
}

int
main(int argc, char *argv[])
{
    int i, pipe_right[2];

    pipe(pipe_right);
    if(fork() == 0){
        // redirect pipe_right[0] to standard input file descriptor 0.
        redirect(pipe_right);
        sieve();
    }else{
        // root parent process feeding number(2-35).
        close(pipe_right[0]);
        for(i = 2; i <= 35; i++){
            write(pipe_right[1], &i, sizeof(int));
        }
        close(pipe_right[1]);
        wait();
    }
    exit();
}