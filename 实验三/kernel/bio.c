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

#define NBUCKETS 13

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hashbucket[NBUCKETS]; //利用哈希表存取不同key值得缓存块，每个哈希桶配备一个lock锁
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int count = 0; count < NBUCKETS; count++){
    initlock(&bcache.lock[count], "bcache"); // 初始化每个哈希桶的lock锁
    bcache.hashbucket[count].prev = &bcache.hashbucket[count]; // hashbucket[count]的prev连接hashbucket[count]
    bcache.hashbucket[count].next = &bcache.hashbucket[count]; // hashbucket[count]的next连接hashbucket[count]
  }
  // Create linked list of buffers
  // 用prev和next节点连接bcache.hashbucket[count]和bcache.buf节点，为每个哈希桶分配等数量缓存块
  int count = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++,count++){
    count = count % NBUCKETS;
    b->next = bcache.hashbucket[count].next;
    b->prev = &bcache.hashbucket[count];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[count].next->prev = b;
    bcache.hashbucket[count].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int currentCount = blockno % NBUCKETS;// 得到当前哈希桶号

  acquire(&bcache.lock[currentCount]);//当前哈希桶加锁

  // Is the block already cached?
  // 先从当前哈希桶中找到命中缓存块
  for(b = bcache.hashbucket[currentCount].next; b != &bcache.hashbucket[currentCount]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[currentCount]);//当前哈希桶解锁
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // 若从当前哈希桶中没有命中缓存块，再从当前哈希桶中找到一个空闲缓存
  for(b = bcache.hashbucket[currentCount].prev; b != &bcache.hashbucket[currentCount]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[currentCount]);//当前哈希桶解锁
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // 若当前哈希桶中找不到空闲缓存，则从其他哈希桶中找到一个空闲缓存存入该哈希桶列表头部
  for(int findCount = currentCount + 1; findCount != currentCount; findCount++){
    findCount = findCount % NBUCKETS;
    if(bcache.lock[findCount].locked){//如果当前锁锁住就跳过
      continue;
    }
    acquire(&bcache.lock[findCount]);//搜查空闲缓存块哈希桶加锁
    for(b = bcache.hashbucket[findCount].prev; b != &bcache.hashbucket[findCount]; b = b->prev){
      if(b->refcnt == 0) {
        //将该缓存移入当前哈希桶头部
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.hashbucket[currentCount].next;
        b->prev = &bcache.hashbucket[currentCount];
        bcache.hashbucket[currentCount].next->prev = b;
        bcache.hashbucket[currentCount].next = b;

        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&bcache.lock[findCount]);//搜查空闲缓存块哈希桶解锁
        release(&bcache.lock[currentCount]);//当前哈希桶解锁
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[findCount]);//搜查空闲缓存块哈希桶解锁
  }
  release(&bcache.lock[currentCount]);//当前哈希桶解锁
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
  int currentCount = b->blockno % NBUCKETS;// 得到当前哈希桶号
  
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  acquire(&bcache.lock[currentCount]);// 当前哈希桶加锁
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    //将该缓存移入当前哈希桶头部
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[currentCount].next;
    b->prev = &bcache.hashbucket[currentCount];
    bcache.hashbucket[currentCount].next->prev = b;
    bcache.hashbucket[currentCount].next = b;
  }
  release(&bcache.lock[currentCount]);// 当前哈希桶解锁
}

void
bpin(struct buf *b) {
  int currentCount = b->blockno % NBUCKETS;// 得到当前哈希桶号
  acquire(&bcache.lock[currentCount]);// 当前哈希桶加锁
  b->refcnt++;
  release(&bcache.lock[currentCount]);// 当前哈希桶解锁
}

void
bunpin(struct buf *b) {
  int currentCount = b->blockno % NBUCKETS;// 得到当前哈希桶号
  acquire(&bcache.lock[currentCount]);// 当前哈希桶加锁
  b->refcnt--;
  release(&bcache.lock[currentCount]);// 当前哈希桶解锁
}


