#ifndef SEM_H
#define SEM_H


#include "types.h"
#include "spinlock.h"


#define MAX_SEMS 64
#define SEM_NAME_LEN 32


struct ksem {
char name[SEM_NAME_LEN]; // нуль-терминированное имя или пустая строка
int value; // значение семафора
int ref; // количество процессов, которые открыли этот семафор
int removed; // 1 если вызван unlink
struct spinlock lock; // защита полей
void *chan; // канал для sleep/wakeup, используем адрес структуры
int inuse; // 1 если запись занята
};


void ksem_init_all(void);
int ksem_open(const char *name, int oflag, int value); // возвращает semid или -1
int ksem_unlink(const char *name); // 0 или -1
int ksem_close(int semid); // закрыть дескриптор
int ksem_wait(int semid);
int ksem_post(int semid);


#endif