#include"kernel/types.h"
#include"user/user.h"

int main()
{
    int fds[2];
    int pid = -1;
    pipe(fds);
    pid = fork();
    if(pid == 0)
    {
        close(fds[1]);
        int x = 0;
        while(read(fds[0], &x, sizeof(int)) != 0)
            printf("%d\n", x);
        close(fds[0]);
    }
    else{
        close(fds[0]);
        for(int i = 1; i <= 10; i++)
            write(fds[1], &i, sizeof(int));
        close(fds[1]);
    }
    exit(0);
}