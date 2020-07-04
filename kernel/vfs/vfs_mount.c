#include <kernel/kassert.h>
#include <kernel/kmem.h>
#include <kernel/vfs/vfs_node.h>
#include <kernel/vfs/vfs_mount.h>

list_t vfs_mounts;

void vfs_mount_init(void) {
}

vfs_mount_t* vfs_mount_create(const char *fs_name, struct vfs_node_s *mounted_on) {
    // FIXME Find FS ops from table of supported file systems

    vfs_mount_t *mnt = (vfs_mount_t*)kmem_alloc(sizeof(vfs_mount_t));
    mnt->lock = SPINLOCK_INIT;
    mnt->mounted_on = mounted_on;
    mounted_on->mounted_here = mnt;
    vfs_node_ref(mounted_on);

    if (mnt->ops->mount != NULL) mnt->ops->mount(mnt);
}

void vfs_mount_destroy(vfs_mount_t *mnt) {
    kassert(mnt != NULL);

    spinlock_writeacquire(&mnt->lock);

    kassert(list_remove(&vfs_mounts, &mnt->ll_node));

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
    return list_entry(list_first(&vfs_mounts), vfs_mount_t, ll_node);
}
