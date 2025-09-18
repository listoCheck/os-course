//
// Created by artem on 18.09.2025.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[]) {
    int p[2]; //массив для пайпов 0 - чтение, 1 - запись
    char recv_buf[16];

    pipe(p);
    if (pipe(p) < 0) {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    if (fork() < 0) {
        fprintf(2, "fork failed\n");
        exit(1);
    }
    if (fork() == 0){ //дочерний процесс
        read(p[0], recv_buf, 16);
        printf("%d: received %s\n", getpid(), recv_buf);
        close(p[0]);

        write(p[1], "pong", 16);
        close(p[1]);

    } else { //родительский процесс
        write(p[1], "ping", 16);
        close(p[1]);

        read(p[0], recv_buf, 16);
        printf("%d: received %s\n", getpid(), recv_buf);
        close(p[0]);
    }
    exit(0);
}