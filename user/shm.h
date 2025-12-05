#ifndef USER_SHM_H
#define USER_SHM_H
#define IPC_CREAT 0x1
#define IPC_EXCL 0x2
#define SHM_RDONLY 0x4

int shmget(int key, int size, int flags);
char* shmat(int shmid, int flags);
int shmdt(char *addr);
int shmctl_rm(int shmid);

#endif
