#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "sem.h"


static struct ksem sem_table[MAX_SEMS];


void
ksem_init_all(void)
{
    for(int i = 0; i < MAX_SEMS; i++){
        sem_table[i].name[0] = '\0';
        sem_table[i].value = 0;
        sem_table[i].ref = 0;
        sem_table[i].removed = 0;
        sem_table[i].chan = &sem_table[i];
        sem_table[i].inuse = 0;
        initlock(&sem_table[i].lock, "ksem");
    }
}


// helper: find by name, caller должен держать семафор на entry? мы будем acquire внутри
static int
find_sem_by_name(const char *name)
{
    for(int i = 0; i < MAX_SEMS; i++){
        acquire(&sem_table[i].lock);
        if(sem_table[i].inuse && strcmp(sem_table[i].name, name) == 0){
            release(&sem_table[i].lock);
            return i;
        }
        release(&sem_table[i].lock);
    }
    return -1;
}


int
ksem_open(const char *name, int oflag, int value)
{
    // oflag: O_CREAT (1), O_EXCL (2)
    int O_CREAT = 1;
    int O_EXCL = 2;


    // First try find existing
    int idx = find_sem_by_name(name);
    if(idx >= 0){
        // exists
        if((oflag & O_EXCL) && (oflag & O_CREAT))
            return -1; // EEXIST semantic


        // increment refcount
        acquire(&sem_table[idx].lock);
        sem_table[idx].ref++;
        release(&sem_table[idx].lock);
        return idx;
    }


    // not found -> if O_CREAT try to allocate
    if(!(oflag & O_CREAT))
        return -1; // ENOENT


    // allocate a free slot
}