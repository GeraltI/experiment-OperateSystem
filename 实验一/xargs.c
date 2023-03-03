#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    int i,j,start;
    int statu;
    int ret;
    char stdin[512],*Argv[MAXARG],*q;
    if(argc <= 1){
        printf("find needs at least one argument!\n"); //检查参数数量是否正确
        exit(-1);
    }
    for(i = 1; i < argc; i++){
        Argv[i - 1] = argv[i];
    }
    while(1){
        gets(stdin,512);
        start = 0;
        for(i = 0; i < 512; i++){
            if(stdin[i] != ' '){
                start = i;
                break;
            }
        }
        if(stdin[start] == '\0'){
            break;
        }
        ret = fork();
        if(ret == 0){

            //子进程

            j = argc - 1;
            q = Argv[j];
            for(i = start; i < 512; i++){
                if(stdin[i] != ' ' && stdin[i] != '\n' && stdin[i] != '\0'){
                    if(i == start || stdin[i - 1] == ' '){
                        char *buf = malloc(512 * (sizeof stdin[i]));
                        Argv[j] = buf;
                        q = buf;
                    }
                    else{
                        q++;
                    }
                    *q = stdin[i];
                }
                else if(stdin[i] == ' '){
                    if(stdin[i + 1] != ' '){
                        q++;
                        *q = '\0';
                        j++;
                    }
                }
                else{
                    if(stdin[i - 1] != ' '){
                        q++;
                        *q = '\0';
                        j++;
                    }
                    Argv[j] = 0;
                    break;
                }
            }
            exec(Argv[0],Argv);
            exit(0);
        }
        else{

            //父进程

            wait(&statu);
        }
    }
    exit(0);
}