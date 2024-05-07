struct filesystem_type
{
    char*  name;
    struct vfs_operations* ops;
    struct inode_operations* iops;
};

/*
This structure stores operations that affect the entire filesystem, and after almost
every operations, there is a change over the state of the filesystem.
*/
struct vfs_operations
{
    int             (*fs_init)(void);
    int             (*mount)(struct inode*, struct inode*);
    struct inode*   (*getroot)(int, int);
    struct inode*   (*ialloc)(uint dev, short type);
    uint            (*balloc)(uint dev);
    void            (*readsb)(int dev, struct superblock* sb);
};

int                     register_fs(struct filesystem_type* fs);
struct filesystem_type* getfs(const char * fs_name);

struct inode_operations
{
   struct inode*    (*dirlookup)(struct inode* dp , char* name, uint* off);
};

