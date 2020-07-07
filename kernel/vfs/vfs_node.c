#include <kernel/kassert.h>
#include <kernel/kmem.h>
#include <kernel/hash.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/vfs/vfs_mount.h>
#include <kernel/vfs/vfs_node.h>

#define VN_AVAIL_MAX             (1024) // 1024 vfs_nodes in circulation

#define VFS_NODE_HASH(mnt, id)   (hash64_fnv1a_pair((unsigned long)mnt, id) % vfs_node_hash_table.num_buckets)

typedef struct {
    spinlock_t lock;       // Multiple readers, single writer lock
    size_t vn_avail;       // Number of vfs_nodes available for allocation
    list_t *ll_vn_buckets; // A list for each hash table buckets
    size_t num_buckets;    // Number of hash table buckets
} vfs_node_hash_table_t;

vfs_node_hash_table_t vfs_node_hash_table;

list_compare_result_t _vfs_node_find(list_node_t *n1, list_node_t *n2) {
    vfs_node_t *v1 = list_entry(n1, vfs_node_t, ll_vnode), *v2 = list_entry(n2, vfs_node_t, ll_vnode);
    return (v1->id == v2->id && v1->mount == v2->mount) ? LIST_COMPARE_EQ : LIST_COMPARE_LT;
}

void _vfs_node_free(vfs_node_t *vn) {
    kmem_free((void*)vn, sizeof(vfs_node_t));

    spinlock_writeacquire(&vfs_node_hash_table.lock);
    vfs_node_hash_table.vn_avail++;
    spinlock_writerelease(&vfs_node_hash_table.lock);
}

vfs_node_t* _vfs_node_alloc(void) {
    spinlock_writeacquire(&vfs_node_hash_table.lock);
    kassert(vfs_node_hash_table.vn_avail != 0);
    vfs_node_hash_table.vn_avail--;
    spinlock_writerelease(&vfs_node_hash_table.lock);

    vfs_node_t *vn = (vfs_node_t*)kmem_alloc(sizeof(vfs_node_t));

    return vn;
}

void vfs_node_init(void) {
    vfs_node_hash_table.lock = SPINLOCK_INIT;
    vfs_node_hash_table.vn_avail = VN_AVAIL_MAX;

    // Set up hash table, 1.5 times the max number of vfs_nodes
    vfs_node_hash_table.num_buckets = VN_AVAIL_MAX + (VN_AVAIL_MAX >> 1);
    size_t size = vfs_node_hash_table.num_buckets * sizeof(list_t);

    vfs_node_hash_table.ll_vn_buckets = (list_t*)kmem_alloc(size);
    arch_fast_zero((uintptr_t)vfs_node_hash_table.ll_vn_buckets, size);
}

vfs_node_t* vfs_node_get(struct vfs_mount_s *mnt, vfs_ino_t id) {
    spinlock_readacquire(&vfs_node_hash_table.lock);

    // Lookup hash table for existing node
    unsigned long hash_bkt = VFS_NODE_HASH(mnt, id);

    vfs_node_t n = { .ll_vnode = LIST_NODE_INITIALIZER, .id = id, .mount = mnt };
    list_node_t *node = list_search(&vfs_node_hash_table.ll_vn_buckets[hash_bkt], _vfs_node_find, &n.ll_vnode);
    vfs_node_t *vn = list_entry(node, vfs_node_t, ll_vnode);

    spinlock_readrelease(&vfs_node_hash_table.lock);

    // If it didn't exist in the hash table then allocate a new vfs_node object
    if (vn == NULL) vn = _vfs_node_alloc();
    vfs_node_ref(vn);

    return vn;
}

void vfs_node_put(vfs_node_t *vn) {
    kassert(vn != NULL);

    spinlock_writeacquire(&vn->lock);
    vn->refcnt--;

    if (vn->refcnt == 0) {
        _vfs_node_free(vn);
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
