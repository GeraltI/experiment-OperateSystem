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

struct kmem {
  struct spinlock lock;
  struct run *freelist;
} ;

struct kmem kmems[NCPU]; // 为每个CPU构建单独的kmem结构体，具备单独的锁和空闲内存空间链表

void
kinit()
{
  for(int id = 0; id < NCPU ; id++){
    initlock(&kmems[id].lock, "kmem"); // 初始化kmems的lock锁
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off(); //关闭中断
  int cpu_id = cpuid(); // 得到当前运行的CPUid号
  pop_off(); //打开中断
  acquire(&kmems[cpu_id].lock); // 对id号CPU的空闲内存空间链表加锁
  r->next = kmems[cpu_id].freelist;// 将该页加入到当前CPU的空闲内存空间链表
  kmems[cpu_id].freelist = r;
  release(&kmems[cpu_id].lock); // 解锁
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int cpu_id = cpuid();
  pop_off();
  acquire(&kmems[cpu_id].lock); //对当前CPU的空闲内存空间链表加锁
  r = kmems[cpu_id].freelist;
  if(r) // 当前CPU的空闲内存空间链表拥有空闲内存
    kmems[cpu_id].freelist = r->next; // 后移空闲内存空间链表
  release(&kmems[cpu_id].lock);
  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    return (void*)r;
  } // 当前CPU的空闲内存空间链表没有空闲内存空间
  for(int id = 0; id < NCPU; id++){// 从其他CPU的内存空间链表中找到空闲内存空间
    if(id == cpu_id)
      continue;
    acquire(&kmems[id].lock);// 对其他CPU的空闲内存空间链表加锁
    r = kmems[id].freelist;
    if(r)
      kmems[id].freelist = r->next;
    release(&kmems[id].lock);//解锁
    if(r){
      memset((char*)r, 5, PGSIZE); // fill with junk
      return (void*)r;
    }
  }
  return 0;// 若没有找到空闲内存空间则返回0
}
