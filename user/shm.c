#include "types.h"
#include "stat.h"
#include "user.h"
#include "shm.h"
int
shmget(int key, int size, int flags)
{
    return syscall(SYS_shmget, key, size, flags);
}
char*
shmat(int shmid, int flags)
{
    int r = syscall(SYS_shmat, shmid, flags);
    if(r == 0) return 0;
    return (char*)r;
}
int
shmdt(char *addr)
{
    return syscall(SYS_shmdt, addr);
}
int
shmctl_rm(int shmid)
{
    return syscall(SYS_shmctl, shmid, 0);
}
