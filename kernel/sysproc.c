#include "types.h" 
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sem.h"
#include "shm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_ksem_open(void)
{
    char name[32];
    int oflag;
    int value;
    if (argstr(0, name, sizeof(name)) < 0)
        return -1;
    argint(1, &oflag);   // argint returns void — сравнивать нельзя
    argint(2, &value);
    return ksem_open(name, oflag, value);
}

uint64 
sys_ksem_unlink(void)
{
    char name[32];
    if (argstr(0, name, sizeof(name)) < 0)
        return -1;
    return ksem_unlink(name);
}

uint64
sys_ksem_close(void)
{
    int semid;
    argint(0, &semid);
    return ksem_close(semid);
}

uint64 
sys_ksem_wait(void)
{
    int semid;
    argint(0, &semid);
    return ksem_wait(semid);
}

uint64 
sys_ksem_post(void)
{
    int semid;
    argint(0, &semid);
    return ksem_post(semid);
}

uint64
sys_shmget(void)
{
    int key, size, flags;
    argint(0, &key);
    argint(1, &size);
    argint(2, &flags);

    return shmget(key, size, flags);
}

uint64
sys_shmat(void)
{
    int shmid, flags;
    argint(0, &shmid);
    argint(1, &flags);

    void *addr = shmat(shmid, flags);
    return (uint64)addr;
}

uint64
sys_shmdt(void)
{
    uint64 addr;
    argaddr(0, &addr);

    return shmdt((void*)addr);
}

uint64
sys_shmctl(void)
{
    int shmid, cmd;
    argint(0, &shmid);
    argint(1, &cmd);

    // пока поддерживаем только удаление сегмента:
    return shmctl_rm(shmid);
}

