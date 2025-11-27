#ifndef SHM_H
#define SHM_H
#include "types.h"
#include "spinlock.h"
#define MAX_SHM_SEGS 32
#define MAX_PAGES_PER_SEG 64
#define MAX_PROC_SHMMAP 16
#define IPC_CREAT  0x1
#define IPC_EXCL   0x2
#define SHM_RDONLY 0x4
struct shmseg {
    int inuse;
    int key;
    int npages;
    char *pages[MAX_PAGES_PER_SEG];
    int refcount;
    int flags;
    int removed;
    struct spinlock lock;
};

void shm_init(void);
int shmget(int key, int size, int flags);
void* shmat(int shmid, int flags);
int shmdt(void *addr);
int shmctl_rm(int shmid);

#endif // SHM_H

