#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "shm.h"
#include "sem.h"

int main(void) {
    int shmid = shmget(1234, 8192, IPC_CREAT);
    if(shmid < 0){
        write(1, "shmget failed\n", 14);
        exit(1);
    }

    char *p = shmat(shmid, 0);
    if(!p){
        write(1, "shmat failed\n", 13);
        exit(1);
    }

    sem_t s = ksem_open("/s", 1, 1);
    if(s < 0) { 
        write(1, "ksem_open failed\n", 17);
        exit(1); 
    }

    if(fork() == 0){
        // CHILD
        ksem_wait(s);
        p[0] = 'C';
        write(1, "child wrote C\n", 14);
        ksem_post(s);
        ksem_close(s);
        exit(0);
    } else {
        // PARENT
        ksem_wait(s);
        p[0] = 'P';
        write(1, "parent wrote P\n", 15);
        ksem_post(s);

        wait(0); // ждем завершения ребенка

        // вывод финального значения в shared memory
        write(1, "final value in shared memory: ", 29);
        write(1, &p[0], 1);
        write(1, "\n", 1);

        ksem_close(s);
        ksem_unlink("/s");
        shmdt(p);
    }

    exit(0);
}
