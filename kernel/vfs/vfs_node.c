/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/kmem_slab.h>
#include <kernel/hash.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/vfs/vfs_mount.h>
#include <kernel/vfs/vfs_node.h>

// Number of hash table buckets is 1.5 times the number of available VFS nodes
#define VN_AVAIL_MAX             (1024) // 1024 vfs_nodes in circulation
#define NUM_BUCKETS              (VN_AVAIL_MAX + (VN_AVAIL_MAX << 1))
#define VFS_NODE_HASH(mnt, id)   (hash64_fnv1a_pair((uint64_t)mnt, id) % NUM_BUCKETS)

typedef struct {
    lock_t lock[NUM_BUCKETS];      // RW lock per bucket
    list_t ll_vnodes[NUM_BUCKETS]; // A list for each hash table buckets
} vfs_node_hash_table_t;

vfs_node_hash_table_t vfs_node_hash_table;

// Slab for vfs_node_t
#define VFS_NODE_SLAB_NUM (1024)
kmem_slab_t vfs_node_slab;

vfs_node_t vfs_node_template;

list_compare_result_t _vfs_node_find(list_node_t *n1, list_node_t *n2) {
    vfs_node_t *v1 = list_entry(n1, vfs_node_t, ll_vnode), *v2 = list_entry(n2, vfs_node_t, ll_vnode);
    return (v1->id == v2->id && v1->mount == v2->mount) ? LIST_COMPARE_EQ : LIST_COMPARE_LT;
}

void vfs_node_init(void) {
    // Setup the slab for the vfs_node_t structs
    kmem_slab_create(&vfs_node_slab, sizeof(vfs_node_t), VFS_NODE_SLAB_NUM);

    // Initialize the hash table
    arch_fast_zero(vfs_node_hash_table.ll_vnodes, NUM_BUCKETS * sizeof(list_t));
    arch_fast_zero(vfs_node_hash_table.lock, NUM_BUCKETS * sizeof(lock_t));

    lock_init(&vfs_node_template.lock);
    vfs_node_template.id = 0;
    vfs_node_template.flags = 0;
    vfs_node_template.refcnt = 0;
    vfs_node_template.mount = NULL;
    vfs_node_template.mounted_here = NULL;
    vfs_node_template.parent = NULL;
    vfs_node_template.object = NULL;
    vfs_node_template.type = VFS_NODE_TYPE_REG;
    list_init(&vfs_node_template.ll_clean);
    list_init(&vfs_node_template.ll_dirty);
    list_node_init(&vfs_node_template.ll_vnode);
    list_node_init(&vfs_node_template.ll_mnode);
    list_node_init(&vfs_node_template.ll_nnode);
    list_init(&vfs_node_template.ll_nc_refs);
    list_init(&vfs_node_template.ll_nc_prefs);
    // FIXME default ops?
    vfs_node_template.ops = NULL;
    vfs_node_template.data = NULL;
}

vfs_node_t* vfs_node_get(struct vfs_mount_s *mnt, vfs_ino_t id) {
    // Lookup hash table for existing node
    unsigned long hash_bkt = VFS_NODE_HASH(mnt, id);

    lock_acquire_shared(&vfs_node_hash_table.lock[hash_bkt]);

    vfs_node_t n = { .id = id, .mount = mnt };
    list_node_t *node = list_search(&vfs_node_hash_table.ll_vnodes[hash_bkt], _vfs_node_find, &n.ll_vnode);
    vfs_node_t *vn = list_entry(node, vfs_node_t, ll_vnode);

    lock_release_shared(&vfs_node_hash_table.lock[hash_bkt]);

    // If it didn't exist in the hash table then allocate a new vfs_node object
    if (vn == NULL) {
        vfs_node_t *vn = (vfs_node_t*)kmem_slab_alloc(&vfs_node_slab);
        kassert(vn != NULL);
        *vn = vfs_node_template;
    }

    vfs_node_ref(vn);

    return vn;
}

void vfs_node_put(vfs_node_t *vn) {
    kassert(vn != NULL);

    lock_acquire_exclusive(&vn->lock);
    vn->refcnt--;

    if (vn->refcnt == 0) {
        // Remove it from the hash table
        unsigned long hash_bkt = VFS_NODE_HASH(vn->mount, vn->id);

        lock_acquire_shared(&vfs_node_hash_table.lock[hash_bkt]);
        list_remove(&vfs_node_hash_table.ll_vnodes[hash_bkt], &vn->ll_vnode);
        lock_release_shared(&vfs_node_hash_table.lock[hash_bkt]);

        kmem_slab_free(&vfs_node_slab, (void*)vn);
        return;
    }

    lock_release_exclusive(&vn->lock);
}

void vfs_node_ref(vfs_node_t *vn) {
    kassert(vn != NULL);

    lock_acquire_exclusive(&vn->lock);
    vn->refcnt++;
    lock_release_exclusive(&vn->lock);
}
