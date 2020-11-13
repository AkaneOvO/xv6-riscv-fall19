#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXARGS 10
#define MAXWORD 30
#define MAXLINE 100

char whitespace[] = " \t\r\n\v";
void pipecmd(char *argv[],int argc,int loc);
void runcmd(char *argv[],int argc);

void panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(-1);
}

int fork1(void)
{
  int pid;
  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//抄sh读输入
int getcmd(char *buf, int nbuf)
{
  fprintf(2, "@ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

// |->第一个命令command 1执行的结果作为command 2的输入传给command 2
void pipecmd(char *argv[],int argc,int loc){
    argv[loc] = 0;
    int fd[2];
    pipe(fd);
    if(fork1()==0){
        //关闭标准输出，重定向到右边
        close(1);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);
        runcmd(argv,loc-1);
    }else{
        //关闭标准输入，重定向到左边
        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);
        runcmd(argv+loc+1,argc-loc-1);
    }
}

void runcmd(char *argv[],int argc){
    for(int i = 0;i<argc;i++){
        if(strcmp(argv[i],"|") == 0){
            //处理一下|
            pipecmd(argv,argc,i);
        }
    }
    for(int i=0;i<argc;i++){
        if(!strcmp(argv[i],">")){
            // int open(const char * pathname, int flags);
            close(1);
            // O_WRONLY 以只写方式打开文件
            // O_CREAT 若欲打开的文件不存在则自动建立该文件
            open(argv[i+1],O_WRONLY|O_CREATE);
            argv[i] = 0;

        }else if(!strcmp(argv[i],"<")){
            close(0);
            // O_RDONLY 以只读方式打开文件
            open(argv[i+1],O_RDONLY);
            argv[i] = 0;
        }
    }
    exec(argv[0],argv);
}

void setArgs(char *cmd, char* argv[],int* argc){
    //得到所有的word和word个数
    int i = 0;
    int count = 0;
    int end = strlen(cmd);
    while(i<end && cmd[i]!='\n'&&cmd[i]!='\0'){
        //跳过空格
        while(i<end && strchr(whitespace,cmd[i])){
            i++;
        }
        argv[count++] = cmd+i;
        //跳过字符
        while(i<end && strchr(whitespace,cmd[i])==0){
            i++;
        }
        cmd[i] = '\0';
        i++;
    }
    // printf("%d\n",count);
    *argc = count;
    // for(int k=0;k<count;k++){
    //     printf("%s\n",argv[k]);
    // }
}

int main(void)
{
    static char buf[100];
    int fd;
    char* argv[MAXARGS];
    // Ensure that three file descriptors are open.
    while ((fd = open("console", O_RDWR)) >= 0)
    {
        if (fd >= 3)
        {
            close(fd);
            break;
        }
    }

    while(getcmd(buf,sizeof(buf))>=0){
        if(fork1() == 0){
            int argc = -1;
            setArgs(buf,argv,&argc);
            runcmd(argv,argc);
        }
        wait(0);
    }
    exit(0);
}