#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"

// Mutual exclusion lock
struct spinlock {
  uint locked;

  // For debugging:
  char *name;
  struct cpu *cpu;
};

// Прототипы функций spinlock
void initlock(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
int holding(struct spinlock *lk);
void push_off(void);
void pop_off(void);

#endif
