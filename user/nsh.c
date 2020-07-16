#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// Limits.
#define MAXARGS 10

// Core data structure.
struct cmd{
    char *infile;
    char *inefile;
    char *outfile;
    char *outefile;
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
};

static struct cmd leftcmd;
static struct cmd rightcmd;

// Function declare.
void panic(char *s);
int getcmd(char *buf, int nbuf);
void clear(void);
void parse(char **ps, char *es);
void run(void);

int
main(void)
{
    static char buf[100];
    char *es, *s;
    int fd;

    // Ensure that three file descriptor are open.
    while((fd = open("console", O_RDWR)) >= 0){
        if(fd >= 3){
            close(fd);
            break;
        }
    }

    // Read and run input commands.
    clear();
    while(getcmd(buf, sizeof(buf)) >= 0){
        if(fork() == 0){
            s = buf;
            es = buf + strlen(buf);
            parse(&s, es);
            run();
        }
        wait(0);
        clear();
    }
    exit(0);
}

// Panic.
void
panic(char *s)
{
    fprintf(2, "%s\n", s);
    exit(-1);
}

// Prompt.
int
getcmd(char *buf, int nbuf)
{
    fprintf(2, "@ ");
    memset(buf, 0 , nbuf);
    gets(buf, nbuf);
    if(buf[0] == 0){
        // EOF.
        return -1;
    }
    return 0;
}

// Util.
void
clearcmd(struct cmd *cmd)
{
    memset(cmd, 0, sizeof(struct cmd));
}

void
clear(void)
{
    clearcmd(&leftcmd);
    clearcmd(&rightcmd);
}

// Parsing.
char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
    char *s;
    int ret;

    s = *ps;
    while(s < es && strchr(whitespace, *s)){
        s++;
    }
    if(q){
        *q = s;
    }
    ret = *s;
    
    switch(*s){
        case 0:
            break;
        case '|':
        case '<':
        case '>':
            s++;
            break;
        default:
            ret = 'a';
            while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s)){
                s++;
            }
            break;
    }

    if(eq){
        *eq = s;
    }

    while(s < es && strchr(whitespace, *s)){
        s++;
    }
    *ps = s;
    return ret;
}

int
peek(char **ps, char *es, char *toks)
{
    char *s;

    s = *ps;
    while(s < es && strchr(whitespace, *s)){
        s++;
    }
    *ps = s;
    return *s && strchr(toks, *s);
}

void
parse1(char **ps, char *es, struct cmd *cmd)
{
    int argc, tok, flag;
    char *q, *eq;

    flag = 0;
    argc = 0;
    while(!peek(ps, es, "|")){
        // EOF.
        if((tok = gettoken(ps, es, &q, &eq)) == 0){
            break;
        }

        // Make sure that the first token could be a command.
        if(!flag){
            flag = 1;
            if(tok != 'a'){
                panic("syntax.");
            }
        }

        cmd->argv[argc] = q;
        cmd->eargv[argc] = eq;
        argc++;
        if(argc >= MAXARGS){
            panic("too many arguments.");
        }

        // Redirection.
        while(peek(ps, es, "<>")){
            // Obtain the redirec symbol.
            tok = gettoken(ps, es, 0, 0);
            if(gettoken(ps, es, &q, &eq) != 'a'){
                panic("missing file for redirection.");
            }
            switch(tok){
                case '<':
                    cmd->infile = q;
                    cmd->inefile = eq;
                    break;
                case '>':
                    cmd->outfile = q;
                    cmd->outefile = eq;
                    break;
            }
        }
    }
    cmd->argv[argc] = 0;
    cmd->eargv[argc] = 0;
}

void
parse(char **ps, char *es)
{
    parse1(ps, es, &leftcmd);
    if(peek(ps, es, "|")){
        // Eliminate the Pipe symbol '|'.
        gettoken(ps, es, 0, 0);
        parse1(ps, es, &rightcmd);
    }
}

// Running.
void
handle_redir(struct cmd *cmd)
{
    // String terminate normally;
    if(cmd->inefile){
        // Input redirection.
        *cmd->inefile = 0;
        close(0);
        if(open(cmd->infile, O_RDONLY) < 0){
            fprintf(2, "open %s failed\n", cmd->infile);
            exit(-1);
        }
    }
    if(cmd->outefile){
        // Output redirection.
        *cmd->outefile = 0;
        close(1);
        if(open(cmd->outfile, O_WRONLY | O_CREATE) < 0){
            fprintf(2, "open %s failed\n", cmd->outfile);
            exit(-1);
        }
    }
}

void
run(void)
{
    int p[2], i;
    if(rightcmd.argv[0] != 0){
        if(pipe(p) < 0){
            panic("pipe failed.");
        }

        if(fork() == 0){
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            if(leftcmd.argv[0] == 0){
                exit(-1);
            }
            handle_redir(&leftcmd);
            // String terminate normally.
            for(i = 0; leftcmd.argv[i]; i++){
                *leftcmd.eargv[i] = 0;
            }
            exec(leftcmd.argv[0], leftcmd.argv);
            fprintf(2, "exec %s failed\n", leftcmd.argv[0]);
        }

        if(fork() == 0){
            close(0);
            dup(p[0]);
            close(p[0]);
            close(p[1]);
            if(rightcmd.argv[0] == 0){
                exit(-1);
            }
            handle_redir(&rightcmd);
            // String terminate normally.
            for(i = 0; rightcmd.argv[i]; i++){
                *rightcmd.eargv[i] = 0;
            }
            exec(rightcmd.argv[0], rightcmd.argv);
            fprintf(2, "exec %s failed\n", rightcmd.argv[0]);
        }

        close(p[0]);
        close(p[1]);
        wait(0);
        wait(0);
    }else{
        if(fork() == 0){
            if(leftcmd.argv[0] == 0){
                exit(-1);
            }
            handle_redir(&leftcmd);
            // String terminate normally.
            for(i = 0; leftcmd.argv[i]; i++){
                *leftcmd.eargv[i] = 0;
            }
            exec(leftcmd.argv[0], leftcmd.argv);
            fprintf(2, "exec %s failed\n", leftcmd.argv[0]);
        }
        wait(0);
    }
}
