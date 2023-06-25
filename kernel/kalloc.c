// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#define stolen_size 16
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};


// 我们需要做的事情总结下来就是为每个 CPU 维护空闲列表，并当空闲列表空时从其他 CPU 处 “偷取” 空余内存。
// 也就是说，我们需要为我们的每一个列表的锁都调用 initlock，并将名字设置为 kmem 开头
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];
// 根据hind给与，每一个cu一个
void
kinit()
{
  // initlock(&kmem.lock, "kmem");
  for (int i = 0; i < NCPU; i++)
  {
    /* code */
    initlock(&kmem[i].lock, "kmem");
  }
  
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  // 获取当前cpu，当前直接全部吃掉
  push_off();
  int cpu = cpuid();
  pop_off();

  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree_cpu(cpu,p);
}


void kfree(void *pa) { 
  push_off();
  int cpu = cpuid();
  pop_off();
  kfree_cpu(cpu,pa);
}
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree_cpu(int cpu,void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  // huoqu dangqian de cpu
  // 需要终端
  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  // 获取当前线程
  push_off();
  int cpu = cpuid();
  pop_off();

  acquire(&kmem[cpu].lock);
  r = kmem[cpu].freelist;
  if(r){
    kmem[cpu].freelist = r->next;
  release(&kmem[cpu].lock);


  
  }else{
    // 借鉴别人的一些
    for (int i = 0; i < NCPU; i++)
    {
      /* code */
      if (i==cpu)
      {
        /* code */
        continue;
      }
      acquire(&kmem[i].lock);
      r = kmem[i].freelist;
      // 开始stolen
      if (r)
      {
        /* code */
         kmem[i].freelist = r->next;
          release(&kmem[i].lock);
         
         break;
      }
      release(&kmem[i].lock);
      
    }
      release(&kmem[cpu].lock);

    
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
