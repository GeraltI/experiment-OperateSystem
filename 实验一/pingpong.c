#include "kernel/types.h"
#include "user/user.h"

#define MAX 128

int main(int argc,char* argv[]){
    if(argc != 1){
        printf("pingpong needs no argument!\n"); //检查参数数量是否正确
        exit(-1);
    }
    int statu;
    int pipe_child_write_to_parent[2];//子进程写给父进程数据的管道
    int pipe_parent_write_to_child[2];//父进程写给子进程数据的管道
    char buffer[MAX];
    if(pipe(pipe_child_write_to_parent) < 0 || pipe(pipe_parent_write_to_child)){
        printf("pipe creates wrong!\n"); //管道创建错误
        exit(-1);
    }
    int ret = fork();
    if(ret < 0){
        printf("fork creates wrong!\n"); //父子进程创建错误
        exit(-1);
    }
    else if(ret == 0){
        //子进程
        close(pipe_parent_write_to_child[1]);//关闭父进程写给子进程数据的管道的写入端
        close(pipe_child_write_to_parent[0]);//关闭子进程写给父进程数据的管道的读出端

        read(pipe_parent_write_to_child[0],buffer,4);//从管道读数据
        printf("%d: received %s\n",getpid(),buffer);

        write(pipe_child_write_to_parent[1],"pong",4);//写数据进管道

        close(pipe_parent_write_to_child[0]);//关闭父进程写给子进程数据的管道的读出端
        close(pipe_child_write_to_parent[1]);//关闭子进程写给父进程数据的管道的写入端

        exit(0);

    }
    else{
        //父进程
        close(pipe_parent_write_to_child[0]);//关闭父进程写给子进程数据的管道的读出端
        close(pipe_child_write_to_parent[1]);//关闭子进程写给父进程数据的管道的写入端

        write(pipe_parent_write_to_child[1],"ping",4);//写数据进管道

        read(pipe_child_write_to_parent[0],buffer,4);//从管道读数据
        printf("%d: received %s\n",getpid(),buffer);

        close(pipe_parent_write_to_child[1]);//关闭父进程写给子进程数据的管道的写入端
        close(pipe_child_write_to_parent[0]);//关闭子进程写给父进程数据的管道的读出端

        wait(&statu);//等待子进程结束
        exit(0);
    }
}
