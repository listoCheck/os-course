#ifndef SEM_H
#define SEM_H

#include "types.h"

#define MAX_SEMS 64
#define SEM_NAME_LEN 32

struct ksem {
    char name[SEM_NAME_LEN];
    int value;
    int ref;
    int removed;
    int inuse;
};

void ksem_init_all(void);
int ksem_open(const char *name, int oflag, int value);
int ksem_unlink(const char *name);
int ksem_close(int semid);
int ksem_wait(int semid);
int ksem_post(int semid);

#endif // SEM_H
