#include "types.h"
#include "stat.h"
#include "user.h"
#include "sem.h"


#define O_CREAT 1
#define O_EXCL 2


sem_t*
sem_open(const char *name, int oflag, int value)
{
    int semid = ksem_open(name, oflag, value);
    if(semid < 0) return 0;
    sem_t *s = malloc(sizeof(sem_t));
    if(!s) return 0;
    s->semid = semid;
    return s;
}


int
sem_close(sem_t *sem)
{
    if(!sem) return -1;
    int r = ksem_close(sem->semid);
    free(sem);
    return r;
}


int
sem_unlink(const char *name)
{
    return ksem_unlink(name);
}


int
sem_wait(sem_t *sem)
{
    if(!sem) return -1;
    return ksem_wait(sem->semid);
}


int
sem_post(sem_t *sem)
{
    if(!sem) return -1;
    return ksem_post(sem->semid);
}