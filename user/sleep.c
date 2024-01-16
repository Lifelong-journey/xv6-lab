#include"kernel/types.h"
#include"user/user.h"

int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("error, here is an exemple: sleep 10\n");
        exit(-1);
    }
    else {
        int sleep_time = atoi(argv[1]);
        if(argv[1][0] == '-'){          
            printf("error, you can't sleep.\n", sleep_time); 
            exit(-1);     
        }
        else
            if(sleep(sleep_time) > 0){
                printf("sleep interrupted\n");
            }
    }
    exit(0);
}