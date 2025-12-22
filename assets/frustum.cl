
struct AABB {
    float3 min;
    float3 max;
};

struct Node {
    struct AABB box;
    uint children[4];
    uint* items;
    uint num_children;
    uint num_Items;  
};

///----------

struct Plane {
    float3 normal;
    float distance;
};

struct Frustum {
    struct Plane planes[6];
};

///----

struct Triangle {
    uint indices[3];
};

struct Mesh {
    struct Triangle* triangles;
    uint num_triangles;
};

///--

static bool is_point_inside_frustum(float3 point, struct Frustum frustum) {
    for (int i = 0; i < 6; i++) {
        struct Plane plane = frustum.planes[i];
        if (dot(plane.normal, point) + plane.distance < 0) {
            return false;
        }
    }
    return true;
}

static bool intersects_aabb_frustum(struct AABB aabb, struct Frustum frustum) {
    for (int i = 0; i < 6; i++) {
        struct Plane plane = frustum.planes[i];
        
        float3 positive_vertex = aabb.min;
        if (plane.normal.x >= 0) positive_vertex.x = aabb.max.x;
        if (plane.normal.y >= 0) positive_vertex.y = aabb.max.y;
        if (plane.normal.z >= 0) positive_vertex.z = aabb.max.z;

        if (dot(plane.normal, positive_vertex) + plane.distance < 0) {
            return false;
        }
    }
    return true;
}

// this called once per quadtree lead node:

/**
 * Creates an index buffer for a given mesh.
 * Supposed to be used for OpenGL IBO generation
 */
__kernel void create_index_buffer(
    __global const struct Mesh* meshes,
    __global const struct Triangle* triangles,
    __global const struct Node* nodes,
    __global const struct Frustum* frustum,
    __global uint* index_buffer,
    __global uint* index_buffer_pointer)
{
    const uint gid = get_global_id(0);
    const uint lid = get_local_id(0);
    const uint lsize = get_local_size(0);

    const __global struct Node* node = &nodes[gid];
    if (!intersects_aabb_frustum(node->box, *frustum))
        return;

    /* ---------------------------
       1) LOCAL COUNT (per thread)
       --------------------------- */
    uint my_count = 0;
    for (uint i = 0; i < node->num_Items; ++i) {
        const __global struct Mesh* mesh = &meshes[node->items[i]];
        my_count += mesh->num_triangles * 3;
    }

    /* ---------------------------
       2) WORKGROUP SCAN
       --------------------------- */
    __local uint l_counts[256];   // must >= local size
    l_counts[lid] = my_count;
    barrier(CLK_LOCAL_MEM_FENCE);

    // inclusive scan
    for (uint offset = 1; offset < lsize; offset <<= 1) {
        uint add = (lid >= offset) ? l_counts[lid - offset] : 0;
        barrier(CLK_LOCAL_MEM_FENCE);
        l_counts[lid] += add;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    const uint my_offset = l_counts[lid] - my_count;
    const uint wg_total  = l_counts[lsize - 1];

    /* ---------------------------
       3) ONE ATOMIC PER WG
       --------------------------- */
    __local uint wg_base;
    if (lid == 0) {
        wg_base = atomic_add(index_buffer_pointer, wg_total);
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    /* ---------------------------
       4) WRITE
       --------------------------- */
    uint write_pos = wg_base + my_offset;

    for (uint i = 0; i < node->num_Items; ++i) {
        const __global struct Mesh* mesh = &meshes[node->items[i]];
        for (uint t = 0; t < mesh->num_triangles; ++t) {
            const __global struct Triangle* tri = &mesh->triangles[t];
            index_buffer[write_pos++] = tri->indices[0];
            index_buffer[write_pos++] = tri->indices[1];
            index_buffer[write_pos++] = tri->indices[2];
        }
    }
}
