#include"kernel/types.h"
#include"user/user.h"

int main()
{
    int fds_par[2];
    int fds_chi[2];
    char buf[100];
    int n = 0;
    int pid = -1;

    pipe(fds_par);
    pipe(fds_chi);
    int fork_pid = fork();
    if(fork_pid == 0) {
        close(fds_par[1]);
        close(fds_chi[0]);

        pid = getpid();
        printf("%d: ", pid);

        write(fds_chi[1], "pong\n", 5);
        close(fds_chi[1]);

        n = read(fds_par[0], buf, sizeof(buf));
        write(1, "received ", 9);
        write(1, buf, n);
        close(fds_par[0]);
    }
    else {
        close(fds_par[0]);
        close(fds_chi[1]);

        write(fds_par[1], "ping\n", 5);
        close(fds_par[1]);

        n = read(fds_chi[0], buf, sizeof(buf));
        pid = getpid();
        printf("%d: ", pid);

        write(1, "received ", 9);
        write(1, buf, n);
        close(fds_chi[0]);
    }
    exit(0);
}