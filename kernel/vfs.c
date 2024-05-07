#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "param.h"
#include "vfs.h"

struct {
  struct spinlock lock;
  struct filesystem_type fss[NFS];
} vfs;

int fsidx = 0;

void 
vfsinit() {
    initlock(&vfs.lock, "vfs");
}

int                     
register_fs(struct filesystem_type* fs) {
    acquire(&vfs.lock);

    if(fsidx >= NFS) {
        return 0;
    }

    struct filesystem_type *t = vfs.fss + fsidx;
    t->name = fs->name;
    t->ops = fs->ops;
    t->iops = fs->iops;

    fsidx++;
    release(&vfs.lock);
    return 1;
}

struct filesystem_type* 
getfs(const char * fs_name) {
    acquire(&vfs.lock);

    struct filesystem_type* t;

    for(t = vfs.fss; t < vfs.fss + NFS; t++) {
        if(strncmp(fs_name, t->name, MAXPATH) == 0) {
            release(&vfs.lock);
            return t;
        }
    }
    release(&vfs.lock);
    return 0;
}