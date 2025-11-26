// kernel/shm.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "shm.h"

// таблица сегментов
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

// helper: find by key (returns index or -1)
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

// Создать или получить shmid по ключу
int
shmget(int key, int size, int flags)
{
  if(size <= 0) size = PGSIZE;
  int npages = (size + PGSIZE - 1) / PGSIZE;
  if(npages > MAX_PAGES_PER_SEG) return -1;

  // ищем существующий
  int idx = find_shm_by_key(key);
  if(idx >= 0){
    // если одновременно были указаны CREATE и EXCL — ошибка
    if((flags & IPC_CREAT) && (flags & IPC_EXCL))
      return -1;
    return idx;
  }

  // если не найден и не просили создавать — ошибка
  if(!(flags & IPC_CREAT)) return -1;

  // выделяем новый сегмент
  for(int i = 0; i < MAX_SHM_SEGS; i++){
    acquire(&shmtab[i].lock);
    if(!shmtab[i].inuse){
      // попробуем выделить npages страниц
      int ok = 1;
      for(int p = 0; p < npages; p++){
        char *pa = kalloc();
        if(!pa){
          ok = 0;
          // откат: освободим уже выделенные
          for(int q = 0; q < p; q++){
            if(shmtab[i].pages[q]){
              kfree(shmtab[i].pages[q]);
              shmtab[i].pages[q] = 0;
            }
          }
          break;
        }
        // очистим страницу
        memset(pa, 0, PGSIZE);
        shmtab[i].pages[p] = pa;
      }
      if(!ok){
        release(&shmtab[i].lock);
        return -1;
      }
      // инициализируем запись
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

  // таблица полна
  return -1;
}

// Присоединить сегмент в адресное пространство текущего процесса.
// Возвращает адрес (va) или 0 при ошибке.
// Предполагается, что struct proc содержит shm_maps[MAX_PROC_SHMMAP] поле.
void*
shmat(int shmid, int flags)
{
  if(shmid < 0 || shmid >= MAX_SHM_SEGS) return 0;

  struct shmseg *s = &shmtab[shmid];
  acquire(&s->lock);
  if(!s->inuse){
    release(&s->lock);
    return 0;
  }

  int readonly = flags & SHM_RDONLY;

  // текущий процесс
  struct proc *p = myproc();

  // ищем свободный слот в p->shm_maps
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

  // выравниваем адрес вверх от p->sz
  uint64 va = PGROUNDUP(p->sz);

  // map pages
for(int i = 0; i < s->npages; i++){
    uint64 pa = (uint64)s->pages[i]; // no V2P in this simplified xv6

    int perm = PTE_U | (readonly ? 0 : PTE_W);

    if(mappages(p->pagetable,
                (uint64)va + i*PGSIZE,
                PGSIZE,
                pa,
                perm) < 0)
    {
        // rollback
        for(int j = 0; j < i; j++){
            uvmunmap(p->pagetable,
                     (uint64)va + j*PGSIZE,
                     1,
                     1);
        }
        release(&s->lock);
        return 0;
    }
}


  // обновляем размер процесса
  p->sz = va + (uint64)s->npages * PGSIZE;

  // увеличиваем refcount и сохраним мэпинг в proc
  s->refcount++;
  p->shm_maps[mapidx].inuse = 1;
  p->shm_maps[mapidx].shmid = shmid;
  p->shm_maps[mapidx].addr = (char*)va;
  p->shm_maps[mapidx].npages = s->npages;

  release(&s->lock);
  return (void*)va;
}

// Отсоединить сегмент, заданный виртуальным адресом addr
int
shmdt(void *addr)
{
  if(!addr) return -1;
  struct proc *p = myproc();

  int mapidx = -1;
  for(int i = 0; i < MAX_PROC_SHMMAP; i++){
    if(p->shm_maps[i].inuse && p->shm_maps[i].addr == addr){
      mapidx = i;
      break;
    }
  }
  if(mapidx < 0) return -1;

  int shmid = p->shm_maps[mapidx].shmid;
  if(shmid < 0 || shmid >= MAX_SHM_SEGS) return -1;

  struct shmseg *s = &shmtab[shmid];
  acquire(&s->lock);

  // unmap virtual pages from this process pagetable
  uvmunmap(p->pagetable, (uint64)addr, p->shm_maps[mapidx].npages, 1);

  // очистим запись в proc
  p->shm_maps[mapidx].inuse = 0;
  p->shm_maps[mapidx].shmid = -1;
  p->shm_maps[mapidx].addr = 0;
  p->shm_maps[mapidx].npages = 0;

  if(s->refcount > 0) s->refcount--;

  // если помечен на удаление и ссылок нет — освобождаем физпамять
  if(s->removed && s->refcount == 0){
    for(int i = 0; i < s->npages; i++){
      if(s->pages[i]){
        kfree(s->pages[i]);
        s->pages[i] = 0;
      }
    }
    s->inuse = 0;
    s->key = 0;
    s->npages = 0;
    s->flags = 0;
    s->removed = 0;
  }

  release(&s->lock);
  return 0;
}

// Удаление сегмента (ctl remove)
int
shmctl_rm(int shmid)
{
  if(shmid < 0 || shmid >= MAX_SHM_SEGS) return -1;
  struct shmseg *s = &shmtab[shmid];
  acquire(&s->lock);
  if(!s->inuse){ release(&s->lock); return -1; }

  s->removed = 1;
  if(s->refcount == 0){
    for(int i = 0; i < s->npages; i++){
      if(s->pages[i]){
        kfree(s->pages[i]);
        s->pages[i] = 0;
      }
    }
    s->inuse = 0;
    s->key = 0;
    s->npages = 0;
    s->flags = 0;
    s->removed = 0;
  }

  release(&s->lock);
  return 0;
}
