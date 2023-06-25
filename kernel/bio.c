// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

// 不通的。我们需要使用一个哈希表来查询块编号，并给每一个 bucket 桶设置一把锁。

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 13
// leetcode lru的优化，使用双线链表转化为hash，hash+双向，就是分了不同的肖bucket
// 区分不同的bucket，bucket是每一个cache，cache使用时间戳来更新lru
// 思路还是双向，只不过是分bucket，使用time来更新

struct
{
  // struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hashtable[NBUCKET];
  struct spinlock lks[NBUCKET];
} bcache;

void binit(void)
{
  struct buf *b;
  // 初始化所有的锁
  // initlock(&bcache.lock, "bcache");
  for (int i = 0; i < NBUCKET; i++)
  {
    /* code */
    initlock(&bcache.lks[i], "bcache");
    // 对bucket来进行
  }

  for (int i = 0; i < NBUCKET; i++)
  {
    /* code */
    bcache.hashtable[i].prev = &bcache.hashtable[i];
    bcache.hashtable[i].next = &bcache.hashtable[i];
  }

  // 对每一个buf，进行设置为单独的节点，不是数组
  // Create linked list of buffers
  //
  // 一次性分配给0号bucket
  /* code */
  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.hashtable[0].next;
    b->prev = &bcache.hashtable[0];
    initsleeplock(&b->lock, "buffer");
    // 初始化每一个buf的
    // 头插法
    bcache.hashtable[0].next->prev = b;
    bcache.hashtable[0].next = b;
  }
}
// 设置hash函数，用于get得到对应的bucket
int hash(uint dev, uint blockno)
{
  return (dev + blockno) % NBUCKET;
}
// 寻找缓冲区
// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = hash(dev, blockno);

  acquire(&bcache.lks[index]);
  // 锁index的bucket

  // 通过head查找不是数组
  // Is the block already cached?
  for (b = bcache.hashtable[index].next; b != &bcache.hashtable[index]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lks[index]);
      // 自旋转锁
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 这里是直接使用时间戳
  // todo: 修改
  // 还是和之前一样，开始查找，没有就开始偷取
  //  使用时间戳来进行比较
  struct buf *victm = 0;
  
  acquire(&tickslock);
  uint minticks = ticks;
  release(&tickslock);
  // 找到最小的，进行替换
  for (b = bcache.hashtable[index].next; b != &bcache.hashtable[index]; b = b->next)
  {
    /* code */
    if (b->refcnt == 0 && b->timestamp <=minticks)
    {
      /* code */
      victm = b;
      minticks = b->timestamp;
    }
  }
  if (!victm)
  {
    // 找不到就开始偷取
    for (int i = 0; i < NBUCKET; i++)
    {
      /* code */
      // 从不同的bucket的查找
      if (i == index)
        continue;
      // huoqu lock
      acquire(&bcache.lks[i]);
      // 接着查找
      /*
      复制上面的代码
      */
      acquire(&tickslock);
      minticks = ticks;
      release(&tickslock);
      
      // 找到最小的，进行替换
      for (b = bcache.hashtable[i].next; b != &bcache.hashtable[i]; b = b->next)
      {
        /* code */
        if (b->refcnt == 0 && b->timestamp <= minticks)
        {
          /* code */
          victm = b;
          minticks = b->timestamp;
        }
      }
       release(&bcache.lks[i]);
      if (victm)
      {
       
        break;
      }
      // 开锁
     
    }
  }

  
  if (victm)
  {
     /* code */
        // 找到就进行break
        // 吧他翘出来，放到inde
        b=victm;
        b->next->prev = b->prev;
        b->prev->next = b->next;

        // 初始化这个锁信息
        // 更新
        b->blockno = blockno;
        b->dev = dev;
        b->refcnt = 1;
        b->valid = 0;

        // 放入到index 头插法
        b->next = bcache.hashtable[index].next;
        bcache.hashtable[index].next->prev = b;
        bcache.hashtable[index].next = victm;
        b->prev = &bcache.hashtable[index];

    acquiresleep(&b->lock);
  
    release(&bcache.lks[index]);
    return b;
  }
  release(&bcache.lks[index]);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");
  // 通过buf获取bucket
  int index = hash(b->dev, b->blockno);
  releasesleep(&b->lock);

  acquire(&bcache.lks[index]);
  b->refcnt--;
  // 不需要进行移动了，更新时间就行
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.hashtable[index].next;
    // b->prev = &bcache.hashtable[index];
    // bcache.hashtable[index].next->prev = b;
    // bcache.hashtable[index].next = b;
    // 最左边的是要进行删除的

    acquire(&tickslock);
    b->timestamp = ticks;
    release(&tickslock);
  }
  // 更新时间

  release(&bcache.lks[index]);
}

void bpin(struct buf *b)
{
  int index = hash(b->dev, b->blockno);
  acquire(&bcache.lks[index]);
  b->refcnt++;
  release(&bcache.lks[index]);
}

void bunpin(struct buf *b)
{
  int index = hash(b->dev, b->blockno);
  acquire(&bcache.lks[index]);
  b->refcnt--;
  release(&bcache.lks[index]);
}
