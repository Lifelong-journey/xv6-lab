#include"kernel/types.h"
#include"user/user.h"
#include"kernel/stat.h"
#include"kernel/fs.h"

char* filename(char* path)
{
    char *p = path + strlen(path);
    while(p >= path && *p != '/')
        p --;
    p ++;
    return p;
}
int find(char *tar, char *path)
{
    //printf("%s\n", path);
    char buf[512];
    int fd;
    struct stat st;
    struct dirent de;
    char *p = 0;

    if((fd = open(path, 0)) < 0) {
        printf("find: cannot open %s\n", path);
        return -1;
    }

    if(fstat(fd, &st) < 0) {
        printf("find: cannot stat %s\n", path);
        close(fd);
        return -1;
    }

    switch(st.type) {
    case T_FILE:   
        if(strcmp(filename(path), tar) == 0){
            printf("%s\n", path);           
        }

    case T_DIR:
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if(de.inum == 0)
                continue;
             memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            if(stat(buf, &st) < 0) {
                printf("find: cannot stat %s\n\n", buf);
                continue;
            }

           
            //printf("%s\n\n", buf);

            if(strcmp(de.name, tar) == 0) {
                printf("%s\n", buf);           
            }

            if(st.type == T_DIR && strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0)           
                if(find(tar, buf) != 0)
                return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("error\n");
        exit(1);
    }
    int isSuc = find(argv[2], argv[1]);
    if(isSuc != 0) {
         printf("find: error accurd\n");
         exit(1);       
    }
    exit(0);
}