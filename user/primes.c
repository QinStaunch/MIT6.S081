#include "kernel/types.h"
#include "user/user.h"

static int pipeline_root[2];

void
util(int* p, int* k)
{
    p[0] = k[0];
    p[1] = k[1];
}

void
sieve()
{
    int n, k;
    int pipeline_left[2], pipeline_right[2];
    util(pipeline_right, pipeline_root);
loop:
    util(pipeline_left, pipeline_right);
    close(pipeline_left[1]);

    if((read(pipeline_left[0], &n, sizeof(int))) <= 0 || n <= 0){
        close(pipeline_left[0]);
        exit();
    }
    printf("prime %d\n", n);

    pipe(pipeline_right);
    // fork right neighbor.
    if((fork() == 0)){
        // in child process.
        goto loop;
    }

    close(pipeline_right[0]);
    while((read(pipeline_left[0], &k, sizeof(int))) > 0 && k > 0){
        if(k % n){
            write(pipeline_right[1], &k, sizeof(int));
        }
    }
    close(pipeline_left[0]);
    close(pipeline_right[1]);
    wait();
    exit();
}

int
main(int argc, char *argv[])
{
    int i;

    pipe(pipeline_root);
    if(fork() == 0){
        sieve();
    }else{
        // parent root process feeding numbers.
        close(pipeline_root[0]);
        for(i = 2; i <= 35; i++){
            write(pipeline_root[1], &i, sizeof(int));
        }
        close(pipeline_root[1]);
        wait();
    }
    exit();
}