/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _VFS_NODE_H_
#define _VFS_NODE_H_

#include <sys/types.h>
#include <kernel/lock.h>
#include <kernel/list.h>
#include <kernel/vm/vm_object.h>

struct vfs_node_s;
struct vfs_mount_s;

typedef uint64_t vfs_ino_t;

typedef enum {
    VFS_NODE_TYPE_REG,
    VFS_NODE_TYPE_DIR,
    VFS_NODE_TYPE_LNK,
} vfs_node_type_t;

typedef enum {
    VFS_MODE_READ,
    VFS_MODE_WRITE,
    VFS_MODE_EXEC,
} vfs_access_mode_t;

typedef struct {
    char *name;       // Pointer to string
    size_t *name_len; // Length of string
} vfs_component_name_t;

typedef struct {
    vfs_node_type_t type;   // Type of file (for create)
    vfs_access_mode_t mode; // File access mode
    unsigned int uid;       // User ID
    unsigned int gid;       // Group ID
    unsigned long fsid;     // File system ID
    size_t block_size;      // Block size used for I/O
    unsigned int num_links; // Number of links to file
    vfs_ino_t id;           // Unique file serial number in file system
    size_t size;            // File size in bytes
    size_t blocks;          // Number of blocks used
    // FIXME
    // struct timeval atime, mtime, ctime, btime
} vfs_node_attr_t;

#define VFS_MAXNAMELEN (255)

typedef struct {
    vfs_ino_t ino;               // Directory serial number
    char name[VFS_MAXNAMELEN+1]; // Entry name
} vfs_dir_entry_t;

typedef unsigned int vfs_node_flags_t;

#define VFS_NODE_FLAGS_ISROOT (0x0001) // This node is the root of it's file system

typedef unsigned int vfs_io_flags_t;

#define VFS_IO_FLAG_UNIT   (0x0001) // Do I/O as atomic unit
#define VFS_IO_FLAG_APPEND (0x0002) // Append writes to end of file
#define VFS_IO_FLAG_SYNC   (0x0004) // Do I/O synchronously
#define VFS_IO_FLAG_ASYNC  (0x0008) // Do I/O asynchronously

// FIXME don't have user credentials yet
typedef struct {
    // Perform close protocol on file
    int (*close)(struct vfs_node_s *vn, vfs_access_mode_t mode);

    // Create a file in the given directory with the given name and attributes. Return the new file in vnp.
    int (*create)(struct vfs_node_s *dvn, vfs_component_name_t *cname, vfs_node_attr_t *attr, struct vfs_node_s **vnp);

    // Create a directory in the given directory with the given name and attributes. Return the new directory in vnp
    int (*create_dir)(struct vfs_node_s *dvn, vfs_component_name_t *cname, vfs_node_attr_t *attr,
        struct vfs_node_s **vnp);

    // Get attributes for file, results are place in attr
    int (*get_attr)(struct vfs_node_s *vn, vfs_node_attr_t *attr);

    // Creates a hard link of vn to the target name cname in the given directory node
    int (*link)(struct vfs_node_s *vn, struct vfs_node_s *dvn, vfs_component_name_t *cname);

    // Lookup component name cname in directory vn, the result will be in vnp
    int (*lookup)(struct vfs_node_s *dvn, vfs_component_name_t *cname, struct vfs_node_s **vnp);

    // Perform open protocol on file
    int (*open)(struct vfs_node_s *vn, vfs_access_mode_t mode);

    // Read count bytes of data into buf from the given offset into the file
    // Returns 0 on EOF or the number of bytes read
    int (*read)(struct vfs_node_s *vn, unsigned int offset, void *buf, size_t count);

    // Reads one file record from the directory at the given offset. Returns the results in dir_entry
    // Also returns 0 on EOF or the number of bytes read
    int (*read_dir)(struct vfs_node_s *dvn, unsigned int offset, vfs_dir_entry_t *dir_entry);

    // Remove a file given by cname in directory dvn
    int (*remove)(struct vfs_node_s *dvn, vfs_component_name_t *cname);

    // Remove directory cname in directory dvn
    int (*remove_dir)(struct vfs_node_s *dvn, vfs_component_name_t *cname);

    // Rename the file sname in directory dvn to tname in directory tvn
    int (*rename)(struct vfs_node_s *dvn, vfs_component_name_t *sname, struct vfs_node_s *tvn,
        vfs_component_name_t *tname);

    // Set attributes for file. Only mode, uid, gid, file size and times may be changed
    int (*set_attr)(struct vfs_node_s *vn, vfs_node_attr_t *attr);

    // Create a symbolic link with name lname in the given directory for the target provided in tname
    int (*sym_link)(struct vfs_node_s *dvn, vfs_component_name_t *lname, vfs_node_attr_t *attr,
        vfs_component_name_t *tname);

    // Write out all dirty buffers; operation is synchronous
    int (*sync)(struct vfs_node_s *vn);

    // Write count bytes of data from buf into the file starting at the given offset
    int (*write)(struct vfs_node_s *vn, unsigned int offset, void *buf, size_t count);
} vfs_node_ops_t;

typedef struct vfs_node_s {
    lock_t lock;                      // RW lock
    vfs_ino_t id;                     // Unique ID (serial number) of this node in it's filesystem
    vfs_node_flags_t flags;           // Various flags
    unsigned long refcnt;             // Number of open files referencing this VFS node
    struct vfs_mount_s *mount;        // File system this node is a part of
    struct vfs_mount_s *mounted_here; // File system that is mounted on this node
    struct vfs_node_s *parent;        // Parent VFS node
    vm_object_t *object;              // Virtual memory object representing this node
    vfs_node_type_t type;             // VFS node type (file, dir, symlink etc.)
    list_t ll_clean;                  // List of clean buffers
    list_t ll_dirty;                  // List of dirty buffers
    unsigned int num_writes;          // Number of writes in progress
    list_node_t ll_vnode;             // VFS node cache hash table bucket linkage
    list_node_t ll_mnode;             // VFS mount node list linkage
    list_node_t ll_nnode;             // Name cache hash table bucket linkage
    list_t ll_nc_refs;                // List of name cache entries that reference this node
                                      // (in case of hard links and '..' and '.')
    list_t ll_nc_prefs;               // List of name cache entries that reference this node as the parent node
    vfs_node_ops_t *ops;              // Pointer to VFS node ops structure
    void *data;                       // File system node specific data
} vfs_node_t;

// Setup the vfs node system
void vfs_node_init(void);

// Either lookup an existing vfs_node using the mnt and ino or, if it doesn't already exist, allocate a new vfs_node
vfs_node_t* vfs_node_get(struct vfs_mount_s *mnt, vfs_ino_t id);

// De-reference a vfs_node, if the reference count is zero this will free the node
void vfs_node_put(vfs_node_t *vn);

// Increment the reference count on the node
void vfs_node_ref(vfs_node_t *vn);

#endif // _VFS_NODE_H_
