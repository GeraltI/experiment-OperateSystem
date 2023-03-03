#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char *path,char* name){
    char buf[512], *p;
    int fd; // 文件名缓冲区
    struct dirent de; // 文件夹结构
    struct stat st; // 文件（包括文件夹）结构

    if((fd = open(path, 0)) < 0){ // 打开path，把文件名存入fd缓冲区
        printf("find: cannot open %s\n", path);
        return;
    }

    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n");
        return;
    }

    strcpy(buf, path); // 复制path给buf
    p = buf + strlen(buf); // 令p指向buf尾并++ 然后 p = '/' ,buf -> buf/ , p指向 '/' 
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0 || strcmp(de.name,".") == 0 || strcmp(de.name,"..") == 0 ){
            continue;
        }  
        memmove(p, de.name, DIRSIZ); // buf(path) / de.name
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
        }
        if(strcmp(de.name,name) == 0){ // de.name == name 则返回buf
            printf("%s\n",buf);
        }
        if(st.type == T_DIR){
            find(buf,name); // 递归find此文件夹
        }
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
    if(argc != 2 && argc != 3 ){
        printf("find needs one or two argument!\n"); //检查参数数量是否正确
        exit(-1);
    }
    if(argc == 2){
        find(".",argv[1]);
    }
    if(argc == 3){
        find(argv[1],argv[2]);
    }
    exit(0);
}