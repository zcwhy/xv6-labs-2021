#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char *argv[])
{
    if(argc != 2) {
        fprintf(2, "sleep <time>\n");
        exit(0);
    }
    printf("%d\n", atoi(argv[1]));
    sleep(atoi(argv[1]));
    
    exit(0);
}