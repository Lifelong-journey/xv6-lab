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

struct Pagecount{ // count for physical memory reference
  struct spinlock lock; //还未了解lock的功能，因此不知是给每一个数组元素都加lock还是给整个数组加lock更好
  int cnt; // 猜测是给元素加lock,因为如果整个加lock大概会影响并发
}ref[(PHYSTOP - KERNBASE) / PGSIZE];

void
kinit()
{
  for (int i = 0; i < (PHYSTOP - KERNBASE) / PGSIZE; i++)
    initlock(&ref[i].lock, "cow");
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    ref[((uint64)p - KERNBASE) / PGSIZE].cnt = 1; //因为kfree会-1
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
  uint64 rpa = ((uint64)pa - KERNBASE) / PGSIZE;
  acquire(&ref[rpa].lock);
  if ((--ref[rpa].cnt) <= 0){
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  }
  release(&ref[rpa].lock);
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
  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk    
    uint64 rpa = ((uint64)r - KERNBASE) / PGSIZE;
    acquire(&ref[rpa].lock);
    if (ref[rpa].cnt)
      panic("kalloc: not a free page");
    ref[rpa].cnt = 1;
    release(&ref[rpa].lock);    
  }

  return (void*)r;
}

// 修改cow计数
void changeref(uint64 pa, uint64 num)
{
  if (pa >= PHYSTOP)
    panic("chgref: pa too big");
  if (pa % PGSIZE != 0)
    panic("chgref: not a complete page");
  uint64 rpa = (pa - KERNBASE) / PGSIZE;
  acquire(&ref[rpa].lock);
  ref[rpa].cnt += num;
  release(&ref[rpa].lock); 
}

// 该page是否是cow page
int iscow(pagetable_t pagetable, uint64 va)
{
  if (va >= MAXVA)
    return -1;
  pte_t* pte = walk(pagetable, va, 0);
  if (!pte)
    return -1;
  if((*pte & PTE_V) == 0)
    return -1;
  return ((*pte) & PTE_F ? 0 : -1);
}

int refcnt(void* pa)
{
  return ref[((uint64)pa - KERNBASE) / PGSIZE].cnt;
}

uint64 cowalloc(pagetable_t pagetable, uint64 va)
{
  if (va % PGSIZE != 0) {
    printf("cowalloc: va exceeds MAXVA\n");
    return -1;
  }

  
  uint64 pa = walkaddr(pagetable, va);
  if (pa == 0)
    return -1;
    //panic("cowalloc: pte not exists");
  pte_t* pte = walk(pagetable, va, 0);  

  if (pte == 0)
    panic("cowalloc: pte not exists");
  if ((*pte & PTE_V) == 0 || (*pte & PTE_U) == 0) {
    panic("cowalloc: pte permission err");
  }


  if (refcnt((void*)pa) == 1) { // 只有一个引用
    *pte |= PTE_W;
    *pte &= ~PTE_F; // 还原标记
    return (uint64)pa;
  } else { // 多个引用
    uint64 mem = (uint64)kalloc();
    if (mem == 0) {
      printf("cowalloc: kalloc fails\n");
      return -1;      
    }

    memmove((void*)mem, (void*)pa, PGSIZE);

    *pte &= ~PTE_V; // 防止被mappage判定为remap

    if (mappages(pagetable, va, PGSIZE, mem, (PTE_FLAGS(*pte) | PTE_W) & ~PTE_F) != 0) {
      kfree((void*)mem);
      *pte |= PTE_V;
      return -1;
    }
    kfree((void*)PGROUNDDOWN(pa)); // 引用减一
    return mem;
  }
}