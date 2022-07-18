#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes(int fd);

int main()
{   
    int p[2];
    pipe(p);

    if(fork() == 0) {
        close(p[1]);
        primes(p[0]);
        close(p[0]);
    } else {
        for(int i = 2; i <= 35; i++) {
            write(p[1], &i, 4);
        }
        close(p[0]);
        close(p[1]);
        wait(0);
    }

    exit(0);
}

void primes(int fd) {
    int prime, num[35], p[2], cnt = 0, cur;
    
    read(fd, &prime, 4);
    printf("prime %d\n", prime);

    while(read(fd, &cur, 4 )) {
        // printf("%d \n",cur);
        if(cur % prime != 0)
            num[cnt++] = cur;
    }

    if(cnt < 1) return ;

    pipe(p);
    if(fork() == 0) {
        close(p[1]);
        primes(p[0]);
        close(p[0]);
    } else {
        write(p[1], num, 4 * cnt);
        close(p[0]);
        close(p[1]);
        wait(0);
    }
}