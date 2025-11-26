#include "types.h"
#include "param.h"
#include "sem.h"
#include "spinlock.h"

// Простые функции для строк
int strcmpsem(const char *p, const char *q) {
    while(*p && (*p == *q)) { p++; q++; }
    return *(unsigned char*)p - *(unsigned char*)q;
}

char *strncpysem(char *dst, const char *src, int n) {
    char *ret = dst;
    while(n-- > 0 && *src) *dst++ = *src++;
    while(n-- > 0) *dst++ = '\0';
    return ret;
}

// Таблица семафоров
static struct ksem sem_table[MAX_SEMS];

// Spinlock для защиты таблицы
static struct spinlock sem_lock;

void ksem_init_all(void) {
    initlock(&sem_lock, "sem_lock");

    acquire(&sem_lock);
    for(int i = 0; i < MAX_SEMS; i++){
        sem_table[i].name[0] = '\0';
        sem_table[i].value = 0;
        sem_table[i].ref = 0;
        sem_table[i].removed = 0;
        sem_table[i].inuse = 0;
    }
    release(&sem_lock);
}

static int find_sem_by_name(const char *name) {
    for(int i = 0; i < MAX_SEMS; i++){
        if(sem_table[i].inuse && strcmpsem(sem_table[i].name, name) == 0)
            return i;
    }
    return -1;
}

int ksem_open(const char *name, int oflag, int value) {
    int O_CREAT = 1;
    int O_EXCL  = 2;

    acquire(&sem_lock);

    int idx = find_sem_by_name(name);
    if(idx >= 0){
        if((oflag & O_EXCL) && (oflag & O_CREAT)) {
            release(&sem_lock);
            return -1;
        }
        sem_table[idx].ref++;
        release(&sem_lock);
        return idx;
    }

    if(!(oflag & O_CREAT)) {
        release(&sem_lock);
        return -1;
    }

    // найти свободный слот
    for(int i = 0; i < MAX_SEMS; i++){
        if(!sem_table[i].inuse){
            sem_table[i].inuse = 1;
            strncpysem(sem_table[i].name, name, SEM_NAME_LEN);
            sem_table[i].name[SEM_NAME_LEN-1] = '\0';
            sem_table[i].value = value;
            sem_table[i].ref = 1;
            sem_table[i].removed = 0;
            release(&sem_lock);
            return i;
        }
    }

    release(&sem_lock);
    return -1;
}

int ksem_close(int semid) {
    if(semid < 0 || semid >= MAX_SEMS) return -1;

    acquire(&sem_lock);
    if(!sem_table[semid].inuse){ release(&sem_lock); return -1; }
    sem_table[semid].ref--;
    if(sem_table[semid].ref == 0 && sem_table[semid].removed){
        sem_table[semid].inuse = 0;
        sem_table[semid].name[0] = '\0';
    }
    release(&sem_lock);
    return 0;
}

int ksem_unlink(const char *name) {
    acquire(&sem_lock);
    int idx = find_sem_by_name(name);
    if(idx < 0){ release(&sem_lock); return -1; }

    sem_table[idx].removed = 1;
    if(sem_table[idx].ref == 0){
        sem_table[idx].inuse = 0;
        sem_table[idx].name[0] = '\0';
    }
    release(&sem_lock);
    return 0;
}

int ksem_wait(int semid) {
    if(semid < 0 || semid >= MAX_SEMS) return -1;

    acquire(&sem_lock);
    if(sem_table[semid].value > 0){
        sem_table[semid].value--;
        release(&sem_lock);
        return 0;
    }
    release(&sem_lock);
    return -1; // нет блокировки в этой версии — просто проверка
}

int ksem_post(int semid) {
    if(semid < 0 || semid >= MAX_SEMS) return -1;

    acquire(&sem_lock);
    sem_table[semid].value++;
    release(&sem_lock);
    return 0;
}
