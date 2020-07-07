#include <kernel/kassert.h>
#include <kernel/kmem.h>
#include <kernel/vfs/vfs_node.h>
#include <kernel/vfs/vfs_mount.h>

typedef struct {
    spinlock_t lock; // List lock
    list_t mounts;   // List of mounts
} vfs_mount_list_t;

vfs_mount_list_t vfs_mount_list;

void vfs_mount_init(void) {
    vfs_mount_list.lock = SPINLOCK_INIT;
    vfs_mount_list.mounts = LIST_INITIALIZER;
}

vfs_mount_t* vfs_mount_create(const char *fs_name, struct vfs_node_s *mounted_on) {
    // FIXME Find FS ops from table of supported file systems

    vfs_mount_t *mnt = (vfs_mount_t*)kmem_alloc(sizeof(vfs_mount_t));
    mnt->lock = SPINLOCK_INIT;
    mnt->mounted_on = mounted_on;
    mounted_on->mounted_here = mnt;
    vfs_node_ref(mounted_on);

    if (mnt->ops->mount != NULL) mnt->ops->mount(mnt);

    spinlock_acquire(&vfs_mount_list.lock);
    kassert(list_insert_last(&vfs_mount_list.mounts, &mnt->ll_node));
    spinlock_release(&vfs_mount_list.lock);
}

void vfs_mount_destroy(vfs_mount_t *mnt) {
    kassert(mnt != NULL);

    spinlock_writeacquire(&mnt->lock);

    spinlock_acquire(&vfs_mount_list.lock);
    kassert(list_remove(&vfs_mount_list.mounts, &mnt->ll_node));
    spinlock_release(&vfs_mount_list.lock);

    if (mnt->mounted_on != NULL) {
        vfs_node_t *mounted_on = mnt->mounted_on;
        mnt->mounted_on = NULL;
        mounted_on->mounted_here = NULL;
        vfs_node_put(mounted_on);
    }

    // FIXME write out all buffers and close nodes

    if (mnt->ops->sync != NULL) mnt->ops->sync(mnt);
    if (mnt->ops->unmount != NULL) mnt->ops->unmount(mnt);

    spinlock_writerelease(&mnt->lock);

    kmem_free(mnt, sizeof(vfs_mount_t));
}

vfs_mount_t* vfs_mount_root(void) {
    return list_entry(list_first(&vfs_mount_list.mounts), vfs_mount_t, ll_node);
}
