#ifndef __VFS_MOUNT_H__
#define __VFS_MOUNT_H__

#include <kernel/spinlock.h>
#include <kernel/list.h>

struct vfs_node_s;
struct vfs_mount_s;

// statvfs flags
#define ST_RDONLY // Read-only file system
#define ST_NOSUID // Does not support semantics of the ST_ISUID and ST_ISGID file mode bits

typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;

struct statvfs {
    unsigned long f_bsize;   // File system block size
    unsigned long f_frsize;  // Fundamental file system block size
    fsblkcnt_t f_blocks;     // Total number of blocks on file system in units of f_frsize
    fsblkcnt_t f_bfree;      // Total number of ree blocks
    fsblkcnt_t f_bavail;     // Number of free blocks avail to non-superuser
    fsfilcnt_t f_files;      // Total number of file nodes in file system
    fsfilcnt_t f_ffree;      // Total number of free file nodes
    fsfilcnt_t f_favail;     // Available file nodes to non-superuser
    unsigned long f_fsid;    // File system id
    unsigned long f_flag;    // File system flags
    unsigned long f_namemax; // Maximum length of filenames
};

typedef struct {
    // Mounts and initializes the given file system
    int (*mount)(struct vfs_mount_s *mnt);

    // Unmounts and de-initializes the given file system
    // After this call the file system should have no open buffers/data/vfs nodes
    int (*unmount)(struct vfs_mount_s *mnt);

    // Get the roout vfs_node for the file system
    int (*root)(struct vfs_mount_s *mnt, struct vfs_node_s **node);

    // Forces all cached data to be written back to the file system. Could potentially be asynchronous
    int (*sync)(struct vfs_mount_s *mnt);

    // Retrieves stats for the file system, results are placed in stat
    int (*stat)(struct vfs_mount_s *mnt, struct statvfs *stat);
} vfs_mount_ops_t;

typedef struct vfs_mount_s {
    spinlock_t lock;
    list_node_t ll_node;           // Linkage for list of all vfs_mounts
    list_t ll_vfs_nodes;           // Linked list of all existing VFS nodes for this mounted file system
    vfs_mount_ops_t *ops;          // VFS mount operations structure
    struct vfs_node_s *mounted_on; // VFS node this file system is mounted on
    size_t block_size;             // File system block size
    void *data;                    // Pointer to file system specific data structure
} vfs_mount_t;

#endif // __VFS_MOUNT_H__
