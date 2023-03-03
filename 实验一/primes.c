#include "kernel/types.h"
#include "user/user.h"

#define MAX 35

int main(int argc,char* argv[]){
    if(argc != 1){
        printf("primes needs no argument!\n"); //检查参数数量是否正确
        exit(-1);
    }

    int ret;
    int statu;

    int num[MAX];

    int pipe_child_write_to_parent[2];
    int pipe_parent_write_to_child[2];
    for(int i = 0; i <= MAX - 1; i++){
        num[i] = i + 2;
    }


    while(num[0] <= MAX){
        printf("prime %d\n",num[0]);
        if(pipe(pipe_child_write_to_parent) < 0 || pipe(pipe_parent_write_to_child)){
            printf("pipe creates wrong!\n"); //管道创建错误
            exit(-1);
        }
        ret = fork();
        if(ret < 0){
            printf("fork creates wrong!\n"); //父子进程创建错误
            exit(-1);
        }
        else if(ret == 0){
            //子进程
            close(pipe_child_write_to_parent[0]);
            close(pipe_parent_write_to_child[1]);

            int numDivide;

            //读父进程数据
            for(int i = 0,j = 0; i <= MAX - 1 ; i++){
                read(pipe_parent_write_to_child[0],(char*)&num[i],4);

                if(num[i] == MAX + 1){
                    num[j] = MAX + 1;
                    break;
                }

                if(i == 0){
                    numDivide = num[i];
                }

                else if(num[i] % numDivide != 0){
                    num[j] = num[i];
                    j++;
                }
            }

            //读数据完成后关闭读数据管道
            close(pipe_parent_write_to_child[0]);

            //写数据给父进程
            for(int i = 0; i <= MAX - 1 ; i++){
                write(pipe_child_write_to_parent[1],(char*)&num[i],4);
                if(num[i] == MAX + 1){
                    break;
                }
            }

            //写数据完成后关闭写数据管道
            close(pipe_child_write_to_parent[1]);
            exit(0);

        }
        else{

            //父进程

            close(pipe_child_write_to_parent[1]);
            close(pipe_parent_write_to_child[0]);

            //写数据给子进程
            for(int i = 0; i <= MAX - 1; i++){
                write(pipe_parent_write_to_child[1],(char*)&num[i],4);
                if(num[i] == MAX + 1){
                    break;
                }
            }

            //写数据完成后关闭写数据管道
            close(pipe_parent_write_to_child[1]);

            //读子进程数据
            for(int i = 0; i <= MAX - 1; i++){
                read(pipe_child_write_to_parent[0],(char*)&num[i],4);
                if(num[i] == MAX + 1){
                    break;
                }
            }

            //读数据完成后关闭读数据管道
            close(pipe_child_write_to_parent[0]);

            wait(&statu);
        }
    }

    exit(0);
}