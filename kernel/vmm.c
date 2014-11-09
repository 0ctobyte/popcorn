#include <kernel/mm.h>

typedef struct vregion {
	vaddr_t start;
	vaddr_t end;
	struct vregion *next;
} vregion_t;

/*typedef struct {

} vmap_t;*/

