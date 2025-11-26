#ifndef USER_SEM_H
#define USER_SEM_H


typedef struct {
int semid;
} sem_t;


sem_t *sem_open(const char *name, int oflag, int value);
int sem_close(sem_t *sem);
int sem_unlink(const char *name);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);


#endif