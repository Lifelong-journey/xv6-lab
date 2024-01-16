#include"kernel/types.h"
#include"user/user.h"
#include"kernel/param.h"

int rcd(char * buf, int n)
{
    int cnt = 0;
    while(1)
    {
        char x;
        int t = read(0, &x, 1);
        if(t == 0)
        {
            return 2;
        }
        if(x == '\r' || x == '\n')
        {
            return 1;
        }
        buf[cnt++] = x;
    }
    return -1;
}
int main(int argc, char *argv[])
{
    char buf[512];
    if(argc < 2){
        printf("error: command format incorret");
        exit(1);        
    }
    char *argv0[MAXARG];
    int argc0 = argc - 1;
    for(int i = 0; i < argc0; i++)
        argv0[i] = argv[i + 1];
    while(1)
    {
        int cnt = argc0;
        int t = 100;
        while(1)
        {
            memset(buf, 0, sizeof buf);
            t = rcd(buf, sizeof buf);
            argv0[cnt++] = buf;
            //printf("buf: %s\n", buf);
            if(t == 2)
                close(0);
            if(t > 0)
                break;
        }
        if(t == 2)
            break;
        if(fork() == 0){
            //printf("argv0: %s %s %s\n", argv0[2], argv0[3], argv0[4]);
            exec(argv[1], argv0);
            printf("exec %s is failed", argv[1]);
            exit(1);
        }
        else
            wait(0);
    }
    exit(0);
}