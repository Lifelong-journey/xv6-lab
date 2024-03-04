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

struct hashbuf {
  struct buf head;
  struct spinlock lock;
};

struct {
  //struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct hashbuf buckets[NBUCKET];
  //struct buf head;
  struct spinlock hashlock;
} bcache;

int hash(int x)
{
  return x % NBUCKET;
}
void
binit(void)
{
  struct buf *b;
  char lockname[16];

  //initlock(&bcache.lock, "bcache");
  initlock(&bcache.hashlock, "hashlock");
  for (int i = 0; i < NBUCKET; i++)
  {
    snprintf(lockname, sizeof(lockname), "bcache-%d", i);
    initlock(&bcache.buckets[i].lock, lockname);
    // Create linked list of buffers
    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
    bcache.buckets[i].head.next = &bcache.buckets[i].head;
  }

  //bcache.head.prev = &bcache.head;
  //bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf + NBUF; b++){
    b->next = bcache.buckets[0].head.next;
    b->prev = &bcache.buckets[0].head;
    initsleeplock(&b->lock, "buffer"); // doubly linked list init
    bcache.buckets[0].head.next->prev = b;
    bcache.buckets[0].head.next = b;
  }
  //printf("binit ok\n");
}

void bsteal(struct buf *b, int i, int bid)
{
  acquire(&bcache.hashlock);
  b->next->prev = b->prev;
  b->prev->next = b->next;
  release(&bcache.hashlock);
  //printf("buckets %d released p6\n", i);  
  //release(&bcache.buckets[i].lock);

  b->next = bcache.buckets[bid].head.next;
  b->prev = &bcache.buckets[bid].head;
  bcache.buckets[bid].head.next->prev = b;
  bcache.buckets[bid].head.next = b;
  return;
  //printf("bsteal ok\n");
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bid = hash(blockno);

  acquire(&bcache.buckets[bid].lock);
  //printf("buckets %d acquired p1\n", bid);

  // Is the block already cached?
  for(b = bcache.buckets[bid].head.next; b != &bcache.buckets[bid].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){ // cashed
      b->refcnt++;

      //acquire(&tickslock); 
      //b->tstamp = ticks; 
      //release(&tickslock); 

      //printf("buckets %d released p2\n", bid);        
      release(&bcache.buckets[bid].lock);
   
      acquiresleep(&b->lock);
      return b;
    }
  }

  //b = 0;
  //struct buf* temp;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for (int j = 0; j < NBUCKET; j++)
  {
    int i = (bid + j) % NBUCKET;
    for(b = bcache.buckets[i].head.prev; b != &bcache.buckets[i].head; b = b->prev){
      if(b->refcnt == 0) {
        if (i != bid)
          bsteal(b, i, bid);
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&bcache.buckets[bid].lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    //int i = (bid + j) % NBUCKET;
    /*if (i != bid) {
      if (!holding(&bcache.buckets[i].lock)) {
        acquire(&bcache.buckets[i].lock);
        //printf("buckets %d acquired p3\n", i);
        //printf("%d\n", bid);
      }
      else
        continue; // avoid deadlock, but little bug
    }*/
    //printf("i go s4 %d\n", i);
    /* uint mx = 0xffffffff;
    for(temp = bcache.buckets[i].head.prev; temp != &bcache.buckets[i].head; temp = temp->prev)
    {
      if (temp->refcnt == 0 && temp->tstamp < mx) {
        b = temp;
        mx = temp->tstamp;
      }
    }
    if(b) {
        if (i != bid)
          bsteal(b, i, bid); // steal

        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        acquire(&tickslock); 
        b->tstamp = ticks; 
        release(&tickslock); 

        //printf("buckets %d released p4 \n", bid);        
        release(&bcache.buckets[bid].lock);
        acquiresleep(&b->lock);
        //printf("i go s1 %d\n", i);
        return b;
      }*/
      //printf("i go s3 %d\n", i);
    //if (holding(&bcache.buckets[i].lock) && i != bid) {
    //  printf("buckets %d released p7\n", i);
    //  release(&bcache.buckets[i].lock);      
    //}
  }

  /* for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }*/
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

  int bid = hash(b->blockno);

  releasesleep(&b->lock);

  acquire(&bcache.buckets[bid].lock);
  //acquire(&bcache.lock);
  b->refcnt--;

  //acquire(&tickslock); 
  //b->tstamp = ticks; 
  //release(&tickslock); 

  if (b->refcnt == 0) { 
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.buckets[bid].head.next;
    b->prev = &bcache.buckets[bid].head;
    bcache.buckets[bid].head.next->prev = b;
    bcache.buckets[bid].head.next = b;
  }
  
  release(&bcache.buckets[bid].lock);
}

void
bpin(struct buf *b) {
  int bid = hash(b->blockno);
  acquire(&bcache.buckets[bid].lock);
  b->refcnt++;
  release(&bcache.buckets[bid].lock);
}

void
bunpin(struct buf *b) {
  int bid = hash(b->blockno);
  acquire(&bcache.buckets[bid].lock);
  b->refcnt--;
  release(&bcache.buckets[bid].lock);
}


