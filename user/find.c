#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *
fmtname(char *path)
{
    static char buf[DIRSIZ + 1]; //0-15
    char *p;

    // Find first character after last slash.
    //从尾部开始搜，直到找到/
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;
    //p指向最后一个/右边的第一个字符

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    //memmove用于复制内存内容
    //void * memmove(void *dest, const void *src, size_t num);
    //将src所指内容的前num个字节复制到dest所指的地址上
    memmove(buf, p, strlen(p));

    //void *memset(void *s, int c, unsigned long n);
    //将s中前n个字节(typedef unsigned int size_t)用ch替换并返回s
    //把后面的全部换成空格
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

void find(char *path, char *name)
{
    //这一段抄ls.c
    //ls可以展示给定目录下所有的文件和目录名称
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    //fstat函数获取已在描述符fields上打开文件的有关信息
    //int fstat(int filedes, struct stat *buf);
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)
    {
    case T_FILE:
        //如果输入的路径包含了文件名，对比一下，一样就输出
        if (strcmp(fmtname(path), name) == 0)
        {
            printf("%s\n", path);
            return;
        }
        break;

    case T_DIR:
        //DIRSIZ is 14->Why?
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }
        //把路径复制给buf
        strcpy(buf, path);
        p = buf + strlen(buf);
        //把输入的路径拼上一个/
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            find(buf, name);
        }
        break;
    }
    close(fd);
}

void main(int argc, char *argv[])
{
    char *path = argv[1];
    char name[DIRSIZ + 1];
    for(int i = 0;i<sizeof(argv[2]);i++){
        name[i] = argv[2][i];
    }
    memset(name + strlen(argv[2]), ' ', DIRSIZ - strlen(argv[2]));
    find(path, name);
    exit();
}