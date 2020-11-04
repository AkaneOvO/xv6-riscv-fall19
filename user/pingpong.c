#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
void main()
{
    int fd_to_child[2];
    int fd_to_parent[2];
    char buf[15];
    pipe(fd_to_child);
    pipe(fd_to_parent);
    //fd0用于读取,fd1用于写入
    write(fd_to_child[1],"received ping\n",14);
    close(fd_to_child[1]);
    if(fork()==0){
        //子进程
        read(fd_to_child[0],buf,sizeof(buf));
        close(fd_to_child[0]);
        printf("%d: %s",getpid(),buf);
        write(fd_to_parent[1],"received pong\n",14);
        close(fd_to_parent[1]);
        exit();
    }else{
        //父进程
        //等待子进程结束
        wait();
        read(fd_to_parent[0],buf,sizeof(buf));
        printf("%d: %s",getpid(),buf);
        close(fd_to_parent[0]);
    }
    exit();
}