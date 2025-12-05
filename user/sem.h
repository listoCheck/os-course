#ifndef USER_SEM_H
#define USER_SEM_H

typedef int sem_t;
sem_t ksem_open(const char *name, int oflag, int value);
int ksem_close(sem_t sem);
int ksem_unlink(const char *name);
int ksem_wait(sem_t sem);
int ksem_post(sem_t sem);

#endif
