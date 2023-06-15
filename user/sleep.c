#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 前面两个是内核态,然后user是进行引入的

int main(int argc, char *argv[]){

    // 参数个数 还有传入的
    if (argc < 2)
    {
        /* code */
        
        exit(0);
    }else{
        // 开始调用
        sleep(atoi(argv[1]));
    }
    exit(0);
}