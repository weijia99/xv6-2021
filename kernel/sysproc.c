#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"
uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


uint64
sys_trace(void)
{
  int mask;
// 这个mask是输入的时候给与的，我们直接使用，不需要自己实现
if (argint(0, &mask) < 0) {  //获取trace的参数
        return -1;               //获取失败返回-1
    }
    return trace(mask); 
 
//  return 0;
}


// implement sys_sysinfo function
uint64
sys_sysinfo(void){
// 使用copy函数来进行复制到st
// addr
   // 剩下的就是复制内容，直接按照提示，看
// ; see sys_fstat() (kernel/sysfile.c) and filestat(
  // 调用kalocate求进程还有free，返回，还有写入
  // 得到当前的进程，然后进行传入数量还有free
    struct sysinfo info;
  struct proc *p;
  uint64 addr;
  // 通过系统获得addr
    if (argaddr(0, &addr) < 0) { //获取系统调用传递的指针参数
      return -1;
  }
  // 把信息传递给info
  p = myproc();
  // 得到当前进行的信息，然后传入到当前进程
  info.freemem= getfreemem();
  info.nproc=procnum();
  // 复制到用户态,复制内存信息到这个的虚拟地址
  // 复制info到这个进程的虚拟地址
  if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0){
    return -1;
  }
  return 0;

}