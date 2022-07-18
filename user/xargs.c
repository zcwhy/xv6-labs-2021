#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char *args[MAXARG], buf[128];
    char *p ,*ep; 
    int argcnt = argc - 1;
    
    p = ep = buf;
    for(int i = 1; i < argc; i++)
        args[i - 1] = argv[i];

    while(read(0, ep, 1)) {
        if(*ep != '\n') {
            // printf("%c ", *ep);
            if(*ep  == ' ') {
                *ep = '\0';
                
                args[argcnt++] = p; 
                p = ++ep;
            } else {
                ep++;
            }
        }
        else {
            *ep = '\0';
            args[argcnt++] = p;
            // for(int i = 0; i < argcnt; i++)
            //     printf("%s\n", args[i]);
            // printf("========\n");
            
            if(fork()) {
                wait(0);
                memset(buf, 0, sizeof buf);
                argcnt = argc - 1;
                p = ep = buf;
            }
            else {
                exec(argv[1], args);                
            }    
        }    
    }
    exit(0);
}