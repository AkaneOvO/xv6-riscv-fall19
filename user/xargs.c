#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

//argc是main函数的参数个数
//argv是main函数的参数序列或指针
void main(int argc, char *argv[]){  
    if (argc < 2) {
        fprintf(2, "no command specified!\n");
        exit();
    }
    //subArgv里存给对应函数的参数和函数名
    //subArgv是一个二维字符串数组
    //MAXARG max exec arguments
    char *subArgv[MAXARG];
    int count = 1;
    //subArgv[0]存了要执行的函数名，后面的是参数
    while(count<argc){
        subArgv[count-1] = argv[count];
        count++;
    }
    char buf[MAXARG*64];
    int bufIndex = 0;
    //从argc的位置开始加参数
    int index = argc-1;
    while(read(0,buf+bufIndex,sizeof(char)) == sizeof(char)){
        //按了回车
        if(buf[bufIndex]=='\n'){
            buf[bufIndex] = 0;
            subArgv[index] = malloc(MAXARG*64);
            memmove(subArgv[index], buf, bufIndex); 
            if(fork()==0){
                //子进程
                // for(int i = 0;i<=index;i++){
                //     printf("%s\n",subArgv[i]);
                // }
                exec(subArgv[0],subArgv);
                //exec执行不返回
            }else{
                //等待子进程结束
                wait();
                index = argc-1;
                bufIndex = 0;
            }
        }else if(buf[bufIndex]==' '){
            //按了空格
            //memmove用于复制内存内容
            //void * memmove(void *dest, const void *src, size_t num);
            //将src所指内容的前num个字节复制到dest所指的地址上
            buf[bufIndex] = 0;
            subArgv[index] = malloc(MAXARG*64);
            memmove(subArgv[index], buf, bufIndex);
            index++; 
            bufIndex = 0;
        }else{
            //读到下一个位置
            //如果超过了缓冲区大小
            if(bufIndex>MAXARG*64)continue;
            bufIndex++;
        }
    }
    exit();
}