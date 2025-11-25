// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

extern char end[]; 
extern struct proc proc[NPROC];
char *page_marks = 0;

static uint64 num_pages = 0;
static char *page_allocated = 0;
static int SAFE_IDX = 0;

int
pa2idx(void *pa) {
  uint64 a = (uint64)pa;
  uint64 base = PGROUNDUP((uint64)end);
  if (a < base || a >= PHYSTOP)
    panic("pa2idx: out of range");
  return (a - base) / PGSIZE;
}


void bd_init(void*, void*);
void bd_free(void*);
void* bd_malloc(uint64);

void
kinit(void) {
  bd_init((char*)PGROUNDUP((uint64)end), (void*)PHYSTOP);

  num_pages = (PHYSTOP - PGROUNDUP((uint64)end)) / PGSIZE;
  if (num_pages == 0)
    panic("kinit: no physical pages?");

  uint64 bytes_needed = num_pages;
  uint64 bytes_per_bitmap = ((bytes_needed + PGSIZE - 1) / PGSIZE) * PGSIZE;
  uint64 total_bytes = bytes_per_bitmap * 2;

  void *block = bd_malloc(total_bytes);
  if (!block)
    panic("kinit: cannot allocate bitmap block");

  page_allocated = (char*)block;
  page_marks = (char*)block + bytes_per_bitmap;

  memset(page_allocated, 0, num_pages);
  memset(page_marks, 0, num_pages);

  if (((uint64)page_allocated % PGSIZE) != 0)
    panic("kinit: bitmap block not page-aligned");

  int block_pages = total_bytes / PGSIZE;
  int start_idx = pa2idx((void*)page_allocated);
  for (int i = 0; i < block_pages; i++) {
    int idx = start_idx + i;
    if (idx < 0 || (uint64)idx >= num_pages) panic("kinit: bitmap block out of range");
    page_allocated[idx] = 1;
  }
  SAFE_IDX = start_idx + block_pages;
  if (SAFE_IDX < 0) SAFE_IDX = 0;
}

void
kfree(void *pa) {
  if (((uint64)pa % PGSIZE) != 0 || (uint64)pa < PGROUNDUP((uint64)end) || (uint64)pa >= PHYSTOP)
    panic("kfree");

  int idx = pa2idx(pa);
  if (!page_allocated[idx])
    panic("kfree: double free or freeing unallocated page");

  memset(pa, 1, PGSIZE);

  page_allocated[idx] = 0;
  page_marks[idx] = 0;

  bd_free(pa);
}

void*
kalloc(void) {
  void *r = bd_malloc(PGSIZE);
  if (r) {
    memset(r, 5, PGSIZE);
    int idx = pa2idx(r);
    page_marks[idx] = 1;
    page_allocated[idx] = 1;
  }
  return r;
}

static void
mark_process_pages_bounded(pagetable_t pagetable, uint64 sz) {
  if (!pagetable) return;
  const uint64 MAX_SCAN = 64;
  uint64 scanned = 0;

  for (uint64 va = 0; va < sz && scanned < MAX_SCAN; va += PGSIZE, scanned++) {
    pte_t *pte = walk(pagetable, va, 0);
    if (!pte) continue;
    if (!(*pte & PTE_V)) continue;
    uint64 pa = PTE2PA(*pte);
    if (pa < PGROUNDUP((uint64)end) || pa >= PHYSTOP) continue;
    int idx = pa2idx((void*)pa);
    page_marks[idx] = 1;
  }

  if (scanned < MAX_SCAN) {
    uint64 stack_top = TRAMPOLINE - PGSIZE;
    uint64 stack_base = stack_top - 2*PGSIZE;
    for (uint64 va = stack_base; va < stack_top && scanned < MAX_SCAN; va += PGSIZE, scanned++) {
      pte_t *pte = walk(pagetable, va, 0);
      if (!pte) continue;
      if (!(*pte & PTE_V)) continue;
      uint64 pa = PTE2PA(*pte);
      if (pa < PGROUNDUP((uint64)end) || pa >= PHYSTOP) continue;
      int idx = pa2idx((void*)pa);
      page_marks[idx] = 1;
    }
  }
}

static int
pa_is_mapped_bounded(uint64 pa) {
  if (pa < PGROUNDUP((uint64)end) || pa >= PHYSTOP)
    return 1;

  const int PROC_LIMIT = 8;
  int seen = 0;

  for (struct proc *p = proc; p < &proc[NPROC] && seen < PROC_LIMIT; p++) {
    if (p->state == UNUSED || !p->pagetable) continue;
    seen++;

    const uint64 MAX_HEAP_SCAN = 16;
    uint64 scanned = 0;
    for (uint64 va = 0; va < p->sz && scanned < MAX_HEAP_SCAN; va += PGSIZE, scanned++) {
      pte_t *pte = walk(p->pagetable, va, 0);
      if (!pte) continue;
      if (!(*pte & PTE_V)) continue;
      if (PTE2PA(*pte) == pa) return 1;
    }

    uint64 stack_top = TRAMPOLINE - PGSIZE;
    uint64 stack_base = stack_top - 2*PGSIZE;
    for (uint64 va = stack_base; va < stack_top; va += PGSIZE) {
      pte_t *pte = walk(p->pagetable, va, 0);
      if (!pte) continue;
      if (!(*pte & PTE_V)) continue;
      if (PTE2PA(*pte) == pa) return 1;
    }
  }
  return 0;
}

void
gc_run(void) {
  int freed = 0;

  if (num_pages == 0 || !page_marks || !page_allocated) {
    printf("GC: not initialized\n");
    return;
  }

  for (uint64 i = 0; i < num_pages; i++)
    page_marks[i] = 0;

  struct proc *me = myproc();
  if (me && me->pagetable) {
    acquire(&me->lock);
    pagetable_t pt = me->pagetable;
    uint64 sz = me->sz;
    release(&me->lock);

    printf("GC: marking pages of current process %d\n", me->pid);
    mark_process_pages_bounded(pt, sz);
  }

  for (struct proc *p = proc; p < &proc[NPROC]; p++) {
    if (p->pid == 1 && p->pagetable) {
      acquire(&p->lock);
      pagetable_t pt = p->pagetable;
      uint64 sz = p->sz;
      release(&p->lock);

      printf("GC: marking pages of init process %d\n", p->pid);
      mark_process_pages_bounded(pt, sz);
      //break;
      return;
    }
  }

  for (uint64 pa = PGROUNDUP((uint64)end); pa < PHYSTOP; pa += PGSIZE) {
    int idx = pa2idx((void*)pa);
    if (idx < SAFE_IDX) continue;

    if (page_allocated[idx] && !page_marks[idx]) {
      if (!pa_is_mapped_bounded(pa)) {
        bd_free((void*)pa);
        page_allocated[idx] = 0;
        freed++;
      }
    }
  }

  printf("GC: freed %d pages\n", freed);
}
