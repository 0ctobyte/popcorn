#include <kernel/kassert.h>
#include <kernel/kmem_slab.h>
#include <kernel/hash.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/vfs/vfs_mount.h>
#include <kernel/vfs/vfs_node.h>

// Number of hash table buckets is 1.5 times the number of available VFS nodes
#define VN_AVAIL_MAX             (1024) // 1024 vfs_nodes in circulation
#define NUM_BUCKETS              (VN_AVAIL_MAX + (VN_AVAIL_MAX << 1))
#define VFS_NODE_HASH(mnt, id)   (hash64_fnv1a_pair((unsigned long)mnt, id) % NUM_BUCKETS)

typedef struct {
    spinlock_t bkt_lock[NUM_BUCKETS];  // Multiple readers, single writer lock per bucket
    list_t ll_vn_buckets[NUM_BUCKETS]; // A list for each hash table buckets
} vfs_node_hash_table_t;

vfs_node_hash_table_t vfs_node_hash_table;

// Slab for vfs_node_t
#define VFS_NODE_SLAB_NUM (1024)
kmem_slab_t vfs_node_slab;

list_compare_result_t _vfs_node_find(list_node_t *n1, list_node_t *n2) {
    vfs_node_t *v1 = list_entry(n1, vfs_node_t, ll_vnode), *v2 = list_entry(n2, vfs_node_t, ll_vnode);
    return (v1->id == v2->id && v1->mount == v2->mount) ? LIST_COMPARE_EQ : LIST_COMPARE_LT;
}

void vfs_node_init(void) {
    // Setup the slab for the vfs_node_t structs
    kmem_slab_create(&vfs_node_slab, sizeof(vfs_node_t), VFS_NODE_SLAB_NUM);

    // Initialize the hash table
    arch_fast_zero((uintptr_t)vfs_node_hash_table.ll_vn_buckets, NUM_BUCKETS * sizeof(list_t));
    arch_fast_zero((uintptr_t)vfs_node_hash_table.bkt_lock, NUM_BUCKETS * sizeof(spinlock_t));
}

vfs_node_t* vfs_node_get(struct vfs_mount_s *mnt, vfs_ino_t id) {
    // Lookup hash table for existing node
    unsigned long hash_bkt = VFS_NODE_HASH(mnt, id);

    spinlock_readacquire(&vfs_node_hash_table.bkt_lock[hash_bkt]);

    vfs_node_t n = { .ll_vnode = LIST_NODE_INITIALIZER, .id = id, .mount = mnt };
    list_node_t *node = list_search(&vfs_node_hash_table.ll_vn_buckets[hash_bkt], _vfs_node_find, &n.ll_vnode);
    vfs_node_t *vn = list_entry(node, vfs_node_t, ll_vnode);

    spinlock_readrelease(&vfs_node_hash_table.bkt_lock[hash_bkt]);

    // If it didn't exist in the hash table then allocate a new vfs_node object
    if (vn == NULL) {
        vfs_node_t *vn = (vfs_node_t*)kmem_slab_alloc(&vfs_node_slab);
        kassert(vn != NULL);
    }

    vfs_node_ref(vn);

    return vn;
}

void vfs_node_put(vfs_node_t *vn) {
    kassert(vn != NULL);

    spinlock_writeacquire(&vn->lock);
    vn->refcnt--;

    if (vn->refcnt == 0) {
        // Remove it from the hash table
        unsigned long hash_bkt = VFS_NODE_HASH(vn->mount, vn->id);

        spinlock_readacquire(&vfs_node_hash_table.bkt_lock[hash_bkt]);
        list_remove(&vfs_node_hash_table.ll_vn_buckets[hash_bkt], &vn->ll_vnode);
        spinlock_readrelease(&vfs_node_hash_table.bkt_lock[hash_bkt]);

        kmem_slab_free(&vfs_node_slab, (void*)vn);
        return;
    }

    spinlock_writerelease(&vn->lock);
}

void vfs_node_ref(vfs_node_t *vn) {
    kassert(vn != NULL);

    spinlock_writeacquire(&vn->lock);
    vn->refcnt++;
    spinlock_writerelease(&vn->lock);
}
