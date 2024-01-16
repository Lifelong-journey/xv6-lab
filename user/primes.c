#include"kernel/types.h"
#include"user/user.h"

int main()
{
    int fds[2];
    int num[40];
    int pid = -1;
    int cnt = 35;
    for(int i = 0; i <= 35; i++)
        num[i] = i + 2;
    for(int i = 1; i <= 33; i++)
    {
        //printf("%d\n", i);
        pipe(fds);//对每一个不超过35层嵌套的进程创建一个向子节点通信的pipe
        pid = fork();//每一个进程都需要fork一个子进程
        if(pid == 0) {//如果是子进程，就读取管道里的数据并写入数组
            cnt = -1;
            int x = 0;
            close(fds[1]);
            while(read(fds[0], &x, sizeof(int)) != 0)
                num[++cnt] = x;
            if(num[0] > 35)
                break;
            close(fds[0]);
        }
        else {//如果是父进程，就要检查当前数组里的数并传入子进程
            close(fds[0]);
            printf("prime %d\n", num[0]);

            for(int j = 1; j <= cnt; j++)
                if(num[j] % num[0] != 0)
                    write(fds[1], &num[j], sizeof(int));
            close(fds[1]);
            wait(0);
            break;
        }
    }
    exit(0);
}