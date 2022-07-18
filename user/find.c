#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void readdir(char *, char *);

char buf[512];
int main(int argc, char *argv[]) {
    if(argc < 3) {
        printf("find <path> <name>\n");
        exit(0);
    }

    char *p;
    struct dirent de;
    struct stat st;

    strcpy(buf, argv[1]);
    p = buf + strlen(buf);
    *p++ = '/';

    int fd = open(argv[1], 0);
    
    while(read(fd, &de, sizeof(de))) {
        if(!strcmp(de.name, ".") || !strcmp(de.name, "..") || de.inum == 0) 
            continue;
        
        strcpy(p, de.name);
        stat(buf, &st);

        switch (st.type)
        {
            case T_FILE :
                if(!strcmp(argv[2], de.name)) 
                    printf("%s\n", buf);
                break;

            case T_DIR :
                // printf("%s %d\n", de.name, de.inum);
                readdir(p+strlen(de.name), argv[2]);
                break;
        }
        
    }    

    close(fd);
    exit(0);
}

void readdir(char *p, char *pattern) {
    struct dirent de;
    struct stat st;
    
    int fd = open(buf, 0);
    
    *p++ = '/';

    // printf("%s\n", buf);


    while(read(fd, &de, sizeof(de))) {
        if(!strcmp(de.name, ".") || !strcmp(de.name, "..") || de.inum == 0) 
            continue;
    
        strcpy(p, de.name);
        stat(buf, &st);

        switch (st.type)
        {
            case T_FILE :
                if(!strcmp(pattern, de.name)) 
                    printf("%s\n", buf);
                break;

            case T_DIR :
                readdir(p+strlen(de.name), pattern);
                break;
        }
        
    }    
    close(fd);
}