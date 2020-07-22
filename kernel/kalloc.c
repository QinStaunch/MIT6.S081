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
  char *pa_start;
  char *ref;
} kmem_ref;

static uint64 kmemindex(void *);

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem_ref.lock, "kmem_ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  uint64 num;

  kmem_ref.ref = (char *)end;
  num = (uint64)(((char *)PHYSTOP-end)>>PGSHIFT) * sizeof(char);
  kmem_ref.pa_start = (char *)PGROUNDUP((uint64)(end + num));
  memset(end, 1, num);
  p = kmem_ref.pa_start;
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

  r = (struct run*)pa;

  acquire(&kmem_ref.lock);
  kmem_ref.ref[kmemindex(pa)]--;
  if(kmem_ref.ref[kmemindex(pa)] > 0){
    release(&kmem_ref.lock);
    return;
  }
  release(&kmem_ref.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

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

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&kmem_ref.lock);
    kmem_ref.ref[kmemindex(r)] = 1;
    release(&kmem_ref.lock);
  }
  return (void*)r;
}

// Util
// Calculate the index of physical address pa in kmem.ref array.
static uint64
kmemindex(void *pa)
{
  return (uint64)(((char *)pa-kmem_ref.pa_start)>>PGSHIFT);
}

// Increment the reference count of physical page at physical address pa.
void
incref(void *pa)
{
  acquire(&kmem_ref.lock);
  kmem_ref.ref[kmemindex(pa)]++;
  release(&kmem_ref.lock);
}
