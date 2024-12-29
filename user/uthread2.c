#include "pthread.h"

int a = 1;

void thread_routine(void *args) {
    printf("thread routine invoking, arg:%d\n", *(int *)args);
}

int main() {
    int tid[2];
    schedinit();
    // printf("tid[0] address is %x, tid[1] address is %x, unc address is %x\n", &tid[0], &tid[1], schduler);
    pthread_create(&tid[0], thread_routine, &a);
    // pthread_create(&tid[1], thread_routine, &a);
    // pthread_create(&tid[1], scheduler, &tid[1]);

    // wait((int *)-1);   
    for(;;) {
        int wpid = wait((int *) 0);
        if(wpid == -1) {
            break;
        }
    }
    // printf("%d\n", a);
    exit(0);
}