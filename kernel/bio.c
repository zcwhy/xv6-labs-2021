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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

struct 
{
  struct spinlock lock;
  struct buf bhead;
}hash_buf[13];

void
binit(void)
{
  // struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  for(int i = 0; i < 13; i++) {
    initlock(&hash_buf[i].lock, "bcache.bucket");
    hash_buf[i].bhead.prev = &hash_buf[i].bhead;
    hash_buf[i].bhead.next = &hash_buf[i].bhead;
  }
    
  for(int i = 0; i < NBUF; i++) {
    int idx = i % 13;

    initsleeplock(&bcache.buf[i].lock, "buffer");
    bcache.buf[i].next = hash_buf[idx].bhead.next;
    bcache.buf[i].prev = &hash_buf[idx].bhead;
    hash_buf[idx].bhead.next->prev = &bcache.buf[i];
    hash_buf[idx].bhead.next = &bcache.buf[i];
  }

  // for(int i = 0; i < 13; i++) {
  //   int cnt = 0;
  //   struct buf *b = hash_buf[i].bhead.next;

  //   while(b != &hash_buf[i].bhead) {
  //     cnt++;
  //     b = b->next;
  //   }

  //   printf("idx = %d, cnt = %d\n", i, cnt);

  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int idx = blockno % 13;
  struct buf *lru_buffer = 0;
  acquire(&hash_buf[idx].lock);
  // acquire(&bcache.lock);

  // Is the block already cached?

  for(b = hash_buf[idx].bhead.next; b != &hash_buf[idx].bhead; b = b->next) {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->lastuse = ticks;
      release(&hash_buf[idx].lock);
      acquiresleep(&b->lock);
      return b;
    } 
  }
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  release(&hash_buf[idx].lock);
  acquire(&bcache.lock);
  int minuse = 1 << 31;
  int bucket = 0;

  for(b = hash_buf[idx].bhead.next; b != &hash_buf[idx].bhead; b = b->next) {
    if(b->dev == dev && b->blockno == blockno){
      acquire(&hash_buf[idx].lock);
      b->refcnt++;
      release(&hash_buf[idx].lock);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    } 
  }
  
  for(int i = 0; i < 13; i++) {
    acquire(&hash_buf[i].lock);
    for(b = hash_buf[i].bhead.next; b != &hash_buf[i].bhead; b = b->next) {
       if(b->refcnt == 0 && b->lastuse <= minuse) {
        lru_buffer = b;
        bucket = i;
      }
    }
    release(&hash_buf[i].lock);
  }

  if(lru_buffer) {
    //delete from orgrnial list
    acquire(&hash_buf[bucket].lock);
    lru_buffer->prev->next = lru_buffer->next;
    lru_buffer->next->prev = lru_buffer->prev;
    release(&hash_buf[bucket].lock);

    //add to new list
    acquire(&hash_buf[idx].lock);
    lru_buffer->next = hash_buf[idx].bhead.next;
    lru_buffer->prev = &hash_buf[idx].bhead;
    hash_buf[idx].bhead.next->prev = lru_buffer;
    hash_buf[idx].bhead.next = lru_buffer;

    //set value
    lru_buffer->dev = dev;
    lru_buffer->blockno = blockno;
    lru_buffer->valid = 0;
    lru_buffer->refcnt = 1;
    release(&hash_buf[idx].lock);
    release(&bcache.lock);
    acquiresleep(&lru_buffer->lock);
    
    return lru_buffer;
  }

  // for(int i = 0; i < 13; i++) {
  //   if(i != idx) {
  //     acquire(&hash_buf[i].lock);
  //     for(b = hash_buf[i].bhead.next; b != &hash_buf[i].bhead; b = b->next){
  //       if(b->refcnt == 0) {
  //         // printf("find\n");
  //         lru_buffer = b;
  //         lru_buffer->dev = dev;
  //         lru_buffer->blockno = blockno;
  //         lru_buffer->valid = 0;
  //         lru_buffer->refcnt = 1;
  //         lru_buffer->lastuse = ticks;

  //         //delete from orgrnial list
  //         lru_buffer->prev->next = lru_buffer->next;
  //         lru_buffer->next->prev = lru_buffer->prev;
  //         //add to new list
  //         lru_buffer->next = hash_buf[idx].bhead.next;
  //         lru_buffer->prev = &hash_buf[idx].bhead;
  //         hash_buf[idx].bhead.next->prev = lru_buffer;
  //         hash_buf[idx].bhead.next = lru_buffer;

  //         release(&hash_buf[i].lock);
  //         release(&hash_buf[idx].lock);
  //         acquiresleep(&lru_buffer->lock);
  //         return lru_buffer; 
  //       }
  //     }
  //     release(&hash_buf[i].lock);
  //   }
  // }
  // printf("bget end2\n");
  

  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  release(&bcache.lock);
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

  acquire(&hash_buf[b->blockno%13].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->lastuse = ticks;
  }
  release(&hash_buf[b->blockno%13].lock);
}

void
bpin(struct buf *b) {
  acquire(&hash_buf[b->blockno%13].lock);
  b->refcnt++;
  release(&hash_buf[b->blockno%13].lock);
}

void
bunpin(struct buf *b) {
  acquire(&hash_buf[b->blockno%13].lock);
  b->refcnt--;
  release(&hash_buf[b->blockno%13].lock);
}


