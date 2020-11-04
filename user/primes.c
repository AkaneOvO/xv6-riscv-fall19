#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void myFunc(int input){
    int divide;
    int tmp;
    int p[2];
    //如果没了
    if(read(input, &divide, sizeof(int)) <= 0){
        exit();
    }
    printf("prime %d\n",divide);
    pipe(p);
    if(fork()==0){
        //创建子进程
        close(p[1]);
        myFunc(p[0]);
        exit();
    }else{
        //父进程
        //关闭读端
        // close(p[0]);
        while(read(input,&tmp,sizeof(int))>0){
            if(tmp%divide != 0){
                // printf("id:%d %d\n",getpid(),tmp);
                write(p[1],&tmp,sizeof(int));
            }
        }
        close(p[1]);
        wait();
        exit();
    }
}

void main(){
    int fd[2];
    pipe(fd);
    int primeNum = 2;
    int nums[36];
    for(int i=3;i<=35;i++){
        nums[i] = i;
    }
    printf("prime %d\n",primeNum);
    if(fork()==0){
        close(fd[1]);
        myFunc(fd[0]);
    }else{
        for(int i = 3;i<=35;i++){
            if(i%2!=0){
                write(fd[1],nums+i,sizeof(int));
            }
        }
        //关闭写端
        close(fd[1]);
        //等待子进程结束
        wait();
        exit();
    }
}