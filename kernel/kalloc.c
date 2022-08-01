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

int page_reference[(PHYSTOP-KERNBASE) >> 12];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    page_reference[((uint64)p - KERNBASE) >> 12] = 1;
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

  if(--page_reference[((uint64)pa - KERNBASE) >> 12] != 0)
    return;
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void 
printf_freepage_cnt()
{
  struct run *r;
  int cnt = 0;

  r = kmem.freelist;
  while(r) {
    cnt++;
    r = r->next;
  }

  printf("%d\n", cnt);
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
  if(r) {
    kmem.freelist = r->next;
    // printf("kalloc-------pa = %p\n", *r);
    // printf_freepage_cnt();
    page_reference[((uint64)r - KERNBASE) >> 12] = 1;
    // printf("pa = %p, ref = %d\n", r, page_reference[(PGROUNDDOWN((uint64)r) - KERNBASE) >> 12]);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void 
incpageref(uint64 pa)
{
  page_reference[((uint64)pa - KERNBASE) >> 12]++;
  // printf("pa = %p, ref = %d\n", pa, page_reference[(PGROUNDDOWN((uint64)pa) - KERNBASE) >> 12]);
}

// void 
// decpageref(uint64 pa)
// {
//   if(--page_reference[(PGROUNDDOWN((uint64)pa) - KERNBASE) >> 12] == 0)
//     kfree((void *) pa);
//   // printf("pa = %p, ref = %d\n", pa, page_reference[(PGROUNDDOWN((uint64)pa) - KERNBASE) >> 12]);
// }

int 
get_pageref(uint64 pa)
{
  return page_reference[(PGROUNDDOWN((uint64)pa) - KERNBASE) >> 12];
}