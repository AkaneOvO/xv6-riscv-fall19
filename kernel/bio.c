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

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

// struct {
//   struct spinlock lock;
//   struct buf buf[NBUF];

//   // Linked list of all buffers, through prev/next.
//   // head.next is most recently used.
//   struct buf head;
// } bcache;

#define NBUCKETS 13

struct
{
  struct buf buf[NBUF*3];
  struct spinlock lock[NBUCKETS];
  struct buf hashbucket[NBUCKETS];
} bcache;

void binit(void)
{
  struct buf *b;

  for(int i=0;i<NBUCKETS;i++){
    initlock(&bcache.lock[i], "bcache");
  }

  // Create linked list of buffers
  for (int i = 0; i < NBUCKETS; i++)
  {
    //初始化还没复制
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }

  //把buf分配到13个桶里
  for(int i=0;i<NBUF*3;i++){
    b = &bcache.buf[i];
    b->next = bcache.hashbucket[i%NBUCKETS].next;
    b->prev = &bcache.hashbucket[i%NBUCKETS];
    initsleeplock(&b->lock,"buffer");
    bcache.hashbucket[i%NBUCKETS].next->prev = b;
    bcache.hashbucket[i%NBUCKETS].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;

  int number = blockno%NBUCKETS;
  acquire(&bcache.lock[number]);

  // Is the block already cached?

  //要用第几块就去对应的桶里找
  for (b = bcache.hashbucket[number].next; b != &bcache.hashbucket[number]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lock[number]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  for (b = bcache.hashbucket[number].prev; b != &bcache.hashbucket[number]; b = b->prev)
  {
    if (b->refcnt == 0)
    {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[number]);
      acquiresleep(&b->lock);
      return b;
    }
  }
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
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int number = b->blockno%NBUCKETS;
  acquire(&bcache.lock[number]);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[number].next;
    b->prev = &bcache.hashbucket[number];
    bcache.hashbucket[number].next->prev = b;
    bcache.hashbucket[number].next = b;
  }

  release(&bcache.lock[number]);
}

void bpin(struct buf *b)
{
  int number = b->blockno%NBUCKETS;
  acquire(&bcache.lock[number]);
  b->refcnt++;
  release(&bcache.lock[number]);
}

void bunpin(struct buf *b)
{
  int number = b->blockno%NBUCKETS;
  acquire(&bcache.lock[number]);
  b->refcnt--;
  release(&bcache.lock[number]);
}
