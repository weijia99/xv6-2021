#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 一对管道，一个child，parent
int main(int argc, char *argv[]){
    int parent[2];
    int child[2];
    // 一遍读，之后才能写入
    pipe(parent);
    pipe(child);
    char word[]={'P'};
    
    // 设置第一个为echo进行输出
    // 全局变量共享，不会因为子进程修改了全局，影响父进程
    if (fork()==0)
    {
        /* code */
        //进行生成进程，现在是孩子进程
        close(parent[1]);
        close(child[0]);
        if (read(parent[0], word, 1) != 1) {
            fprintf(2, "Child: Failed to read from pipe parent2child!\n");
            exit(1);
        }
        printf("%d: received ping\n", getpid());
        if (write(child[1], word, 1) != 1) {
            fprintf(2, "Child: Failed to write to pipe child2parent!\n");
            exit(1);
        }
        exit(0);

        // 0开始读入
    }else{
        // 首先开始写入
        close(parent[0]);
        close(child[1]);
        // word = '!';
        int write_out = write(parent[1],word,1);
        // 写入一个字符,返回结果
        int read_out =  read(child[0],word,1);
        if(write_out==1 && read_out==1){
            printf("%d: received pong\n", getpid());
            exit(0);
        }else{
            exit(1);
        }
        
    }
    
}
