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

#define BUCKET_NUM 13

struct {
  struct buf buf[NBUF];
  struct buf hashtable[BUCKET_NUM];
  struct spinlock lock_bucket[BUCKET_NUM];

  struct spinlock lock_evict;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock_evict, "bcache_evict");

  // Create linked list of buffers
  int i;
  for (i = 0; i < BUCKET_NUM; i++) {
    initlock(&bcache.lock_bucket[i], "bcache");
    bcache.hashtable[i].prev = &bcache.hashtable[i];
    bcache.hashtable[i].next = &bcache.hashtable[i];
  }
  i = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashtable[i].next;
    b->prev = &bcache.hashtable[i];
    initsleeplock(&b->lock, "buffer");
    bcache.hashtable[i].next->prev = b;
    bcache.hashtable[i].next = b;
    if (++i == BUCKET_NUM) {
      i = 0;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int i = blockno % BUCKET_NUM;
  acquire(&bcache.lock_bucket[i]);

  // Is the block already cached?
  for(b = bcache.hashtable[i].next; b != &bcache.hashtable[i]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock_bucket[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.hashtable[i].prev; b != &bcache.hashtable[i]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock_bucket[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // steal
  acquire(&bcache.lock_evict);
  for (int j = 0; j < BUCKET_NUM; j++) {
    if (j == i) continue;
    for (b = bcache.hashtable[j].prev; b != &bcache.hashtable[j]; b = b->prev) {
      if (b->refcnt == 0) {
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.hashtable[i].next;
        b->prev = &bcache.hashtable[i];
        bcache.hashtable[i].next->prev = b;
        bcache.hashtable[i].next = b;

        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&bcache.lock_bucket[i]);
        release(&bcache.lock_evict);
        acquiresleep(&b->lock);
        return b;
      }
    }
  }
  

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int i = b->blockno % BUCKET_NUM;
  acquire(&bcache.lock_bucket[i]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashtable[i].next;
    b->prev = &bcache.hashtable[i];
    bcache.hashtable[i].next->prev = b;
    bcache.hashtable[i].next = b;
  }
  
  release(&bcache.lock_bucket[i]);
}

void
bpin(struct buf *b) {
  int i = b->blockno % BUCKET_NUM;
  acquire(&bcache.lock_bucket[i]);
  b->refcnt++;
  release(&bcache.lock_bucket[i]);
}

void
bunpin(struct buf *b) {
  int i = b->blockno % BUCKET_NUM;
  acquire(&bcache.lock_bucket[i]);
  b->refcnt--;
  release(&bcache.lock_bucket[i]);
}


