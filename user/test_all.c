#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "shm.h"
#include "sem.h"

int main(int argc, char *argv[]) {
    if(argc < 2){
        printf("Usage: %s <role>\n", argv[0]);
        exit(1);
    }

    int shmid = shmget(1234, 8192, IPC_CREAT);
    if(shmid < 0){
        printf("shmget failed\n");
        exit(1);
    }

    char *p = shmat(shmid, 0);
    if(!p){
        printf("shmat failed\n");
        exit(1);
    }

    sem_t s = ksem_open("/s", 1, 1);
    if(s < 0){
        printf("ksem_open failed\n");
        exit(1);
    }

    if(strcmp(argv[1], "A") == 0){
        ksem_wait(s);
        p[0] = 'A';
        printf("Process A wrote A\n");
        ksem_post(s);
    } else if(strcmp(argv[1], "B") == 0){
        ksem_wait(s);
        p[0] = 'B';
        printf("Process B wrote B\n");
        ksem_post(s);
    }
    ksem_wait(s);
    printf("Process %s sees value: %c\n", argv[1], p[0]);
    ksem_post(s);

    ksem_close(s);
    shmdt(p);
    exit(0);
}
