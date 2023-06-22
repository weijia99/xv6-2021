// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
struct {
  struct spinlock lock;
  int refcount_arr[(PHYSTOP-KERNBASE)/PGSIZE];
} refcount;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&refcount.lock, "refcount");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    refcount_add((uint64)p,1,1);
    // 这里要free，就子豪设置初始化为0
    kfree(p);

  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if(refcount_add((uint64)pa,-1,0)>0){
    return;
    // 还有引用直接推出
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  // 设置开始的引用就是1
  if(r){
    refcount_add((uint64)r,1,1);
  }
  return (void*)r;
}


// 设置引用计数，使用数组来统计每一个页面的
int refcount_add(uint64 va, int add,int flag) { 
  int index = (va - KERNBASE) / PGSIZE;
  acquire(&refcount.lock);
  if(flag==1){
    refcount.refcount_arr[index] = add;
  }else{

  
  int res = refcount.refcount_arr[index];
  res += add;
  refcount.refcount_arr[index] = res;
  }
  release(&refcount.lock);
  return refcount.refcount_arr[index];
}