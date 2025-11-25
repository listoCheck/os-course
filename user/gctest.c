#include "kernel/types.h"
#include "user.h"

int main() {
    printf("GC test start\n");

    char *p1 = sbrk(4096);
    char *p2 = sbrk(4096);
    char *p3 = sbrk(4096);

    printf("Allocated 3 pages: %p %p %p\n", p1, p2, p3);

    p1 = 0;
    p2 = 0;
    p3 = 0;

    printf("Calling GC...\n");
    gc();

    printf("GC finished\n");
    exit(0);
}
