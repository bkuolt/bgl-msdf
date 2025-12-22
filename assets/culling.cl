#include "AABB.h"

struct Mesh {
    uint* triangles;
    uint num_Triangles;
}; 


bool contains(const global struct AABB* node, const global struct AABB* aabb) {
    // TODO
    return false;
}

__kernel void cull(
    __global struct Node* nodes,
    __global struct AABB* aabb,
    __global struct Mesh* meshes,
    
    __global int* num_hits,
    __global int* hits
    )
{
    global const struct Node* node = nodes + get_global_linear_id();
    if (!contains(&node->box, aabb)) {
        return;
    }

    // find local hits
    uint local_num_hits = 0;
    uint local_hits[4];

    for (int i = 0; i < node->num_children; ++i) {
        const uint child_index = node->children[i];
        const global struct Node* child = &nodes[child_index];
        if (contains(&child->box, aabb)) {
            local_hits[local_num_hits] = child_index;
            ++local_num_hits;
        }
    }

    if (local_num_hits == 0) {
        return;  // nothing to copy back
    }

    // copy back to global memory
    uint local_hit_index = atomic_add(num_hits, local_num_hits);
    for (int i = 0; i < local_num_hits; ++i) {
        hits[local_hit_index + i] = local_hits[i];
    }
}