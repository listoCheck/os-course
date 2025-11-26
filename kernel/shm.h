#ifndef SHM_H
#define SHM_H
#include "types.h"
#include "spinlock.h"
#define MAX_SHM_SEGS 32
#define MAX_PAGES_PER_SEG 64 // максимум страниц в одном сегменте
#define MAX_PROC_SHMMAP 16
#define IPC_CREAT  0x1
#define IPC_EXCL   0x2
#define SHM_RDONLY 0x4
struct shmseg {
    int inuse; // 1 если запись занята
    int key; // ключ IPC
    int npages; // число страниц
    char *pages[MAX_PAGES_PER_SEG]; // указатели на физические страницы (kalloc())
    int refcount; // количество attach'ов
    int flags; // побитовые флаги (при создании)
    int removed; // пометка на удаление
    struct spinlock lock; // защита структуры
};

 void shm_init(void);
 int shmget(int key, int size, int flags);
 void *shmat(int shmid, int flags);
 int shmdt(void *addr);
 int shmctl_rm(int shmid);
 #endif // SHM_H