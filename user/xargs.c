#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define ARGLEN 32
#define STDIN  0

int
main(int argc, char *argv[])
{
    int i, j, flag, len;
    char c, args[MAXARG][ARGLEN];
    char *m[MAXARG];
    if(argc < 2){
        fprintf(2, "Usage:xargs command ...\n");
        exit();
    }

    while(1){
        // clear the args buf.
        memset(args, 0 ,MAXARG * ARGLEN);
        // reconstruct argv named args for child process.
        for(i = 0, j = 1; j < argc && i < MAXARG - 1; j++){
            strcpy(args[i++], argv[j]);
        }

        j = 0;
        flag = 0; // indicate the first blank.
        while((len = read(STDIN, &c, 1)) > 0 && c != '\n' && i < MAXARG - 1){
            if(c == ' '){
                // skip blank aside from the first blank occured.
                if(flag){
                    i++;
                    j = 0;
                    flag = 0;
                }
                continue;
            }
            args[i][j++] = c;
            flag = 1; // restore flag.
        }

        // indicate CTRL + D.
        if(len <= 0){
            wait();
            exit();
        }

        // convert types.
        // It is necessary for that the type second argument of function exec
        // is 'char **' while the type of variable args is 'char [][32]'.
        for(i = 0; i < MAXARG - 1; i++){
            m[i] = args[i];
        }
        // make ture that the last argument is 0.
        m[MAXARG - 1] = 0;

        if(fork() == 0){
            // in child process.
            exec(m[0], m);
            exit();
        }
    }
    exit();
}