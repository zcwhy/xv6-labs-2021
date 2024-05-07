#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "mount.h"

struct inode* mtablertinode(struct inode * ip) {
    struct inode *rtinode;
    struct mntentry* mp;

    acquire(&mtable.lock);
    for(mp = mtable.mpoint; mp < &mtable.mpoint[MOUNTSIZE]; mp++) {
        if (mp->m_inode->dev == ip->dev && mp->m_inode->inum == ip->inum) {
            rtinode = mp->m_rtinode;
            release(&mtable.lock);

            return rtinode;
        }
    }
    release(&mtable.lock);
    return 0;
}