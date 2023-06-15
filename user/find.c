#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  
  return p;
}
// 实现一个简单的 findUNIX 程序，
// 通过该程序我们可以查找目录树下的给定具体名字的所有文件。完成的程序包存在 user/find.c 中
void find(char *path,char *key){
char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
//   首先打开路径
  if((fd = open(path, 0)) < 0){
    // fprintf(2, "ls: cannot open %s\n", path);
    return;
  }
// 对这个fd进行kan能不能读写
    if(fstat(fd, &st) < 0){
    // fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }
        char *last1;

    switch(st.type){
    case T_FILE:
        // 得到末尾文件
        last1 = fmtname(path);
        if(strcmp(last1,key)==0){
            fprintf(2,"%s\n",path);
        }
        break;
    case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    // 文件夹操作
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0||strcmp(de.name,".")==0||strcmp(de.name,"..")==0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        // printf("ls: cannot stat %s\n", buf);
        continue;
      }
    //   printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
        // 递归查询
        find(buf,key);
    }
     break;
    }
    close(fd);
}

int main(int argc, char *argv[]){
     

  if(argc < 2){
    // find(".");
    exit(0);
  }
  find(argv[1],argv[2]);
//   从z1找到2
  exit(0);

}