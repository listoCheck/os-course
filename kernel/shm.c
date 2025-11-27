// kernel/shm.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "shm.h"

static struct shmseg shmtab[MAX_SHM_SEGS];

void
shm_init(void)
{
  for(int i = 0; i < MAX_SHM_SEGS; i++){
    shmtab[i].inuse = 0;
    shmtab[i].key = 0;
    shmtab[i].npages = 0;
    shmtab[i].refcount = 0;
    shmtab[i].flags = 0;
    shmtab[i].removed = 0;
    for(int j = 0; j < MAX_PAGES_PER_SEG; j++)
      shmtab[i].pages[j] = 0;
    initlock(&shmtab[i].lock, "shmseg");
  }
}

static int
find_shm_by_key(int key)
{
  for(int i = 0; i < MAX_SHM_SEGS; i++){
    acquire(&shmtab[i].lock);
    if(shmtab[i].inuse && shmtab[i].key == key){
      release(&shmtab[i].lock);
      return i;
    }
    release(&shmtab[i].lock);
  }
  return -1;
}

int
shmget(int key, int size, int flags)
{
  if(size <= 0) size = PGSIZE;
  int npages = (size + PGSIZE - 1) / PGSIZE;
  if(npages > MAX_PAGES_PER_SEG) return -1;

  int idx = find_shm_by_key(key);
  if(idx >= 0){
    if((flags & IPC_CREAT) && (flags & IPC_EXCL))
      return -1;
    return idx;
  }

  if(!(flags & IPC_CREAT)) return -1;

  for(int i = 0; i < MAX_SHM_SEGS; i++){
    acquire(&shmtab[i].lock);
    if(!shmtab[i].inuse){
      shmtab[i].inuse = 1;
      shmtab[i].key = key;
      shmtab[i].npages = npages;
      shmtab[i].refcount = 0;
      shmtab[i].flags = flags;
      shmtab[i].removed = 0;
      release(&shmtab[i].lock);
      return i;
    }
    release(&shmtab[i].lock);
  }

  return -1;
}

void* shmat(int shmid, int flags)
{
    if(shmid < 0 || shmid >= MAX_SHM_SEGS) return 0;
    struct shmseg *s = &shmtab[shmid];
    acquire(&s->lock);
    if(!s->inuse){
        release(&s->lock);
        return 0;
    }

    struct proc *p = myproc();
    int mapidx = -1;
    for(int i = 0; i < MAX_PROC_SHMMAP; i++){
        if(!p->shm_maps[i].inuse){
            mapidx = i;
            break;
        }
    }
    if(mapidx < 0){
        release(&s->lock);
        return 0;
    }

    uint64 va = PGROUNDUP(p->sz);
    if(uvmalloc(p->pagetable, va, va + s->npages*PGSIZE, PTE_W | PTE_U) == 0){
        release(&s->lock);
        return 0;
    }

    p->shm_maps[mapidx].inuse = 1;
    p->shm_maps[mapidx].shmid = shmid;
    p->shm_maps[mapidx].addr = (char*)va;
    p->shm_maps[mapidx].npages = s->npages;
    s->refcount++;

    p->sz = va + s->npages*PGSIZE;
    release(&s->lock);
    return (void*)va;
}
int shmdt(void *addr) {
    return 0;
}

int shmctl_rm(int shmid) {
    return 0;
}
