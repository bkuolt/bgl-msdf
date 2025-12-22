#include "AABB.h"

// TODO: clean up header files
// TODO: add header files
// TODO: separate C++ source and headers properly
// TODO: finish frustum culling kernel

__constant sampler_t samp =
    CLK_NORMALIZED_COORDS_FALSE |
    CLK_ADDRESS_CLAMP |
    CLK_FILTER_NEAREST;


struct Vertex {
    float3 position;
    float3 normal;
    float2 texCoord;
};

struct Triangle {
    uint vertices[3];
};


uint get_vertex_id(uint width, int2 coord) {
    return coord.x + coord.y * width;
}

// TODO: 
// input: heightmap
// output: vbo
__kernel void calculate_geometry(
    __global const float* a,
    __global const float* b,
    __global struct Vertex* out,
    read_only image2d_t image)
{

    const uint width = get_image_width(image);
    const uint height = get_image_height(image);

    const uint x = get_global_id(0);
    const uint y = get_global_id(1);

    // get neighbouring coordinates
    const int2 coords[] = {
        (int2)(x, y),
        (int2)(x + 1, y),
        (int2)(x, y + 1),
        (int2)(x + 1, y + 1)
    };

    // create vertices
    struct Vertex vertex[4];

    for (int i = 0; i < 4; ++i) {
        int2 coord = coords[i];

        if (coord.x >= width || coord.y >= height) {
            continue;
        }

        float4 rgba = read_imagef(image, samp, (int2)(x, y));
        float z = rgba.x;

        vertex[i].position = (float3)(coord.x / width, coord.y / height, z);
        vertex[i].texCoord = (float2)(coord.x / width, coord.y / height);

        int gid = get_global_id(0) * 4 + i;
        out[gid] = vertex[i];
    }

    int id = 0;
    
    // create triangles
    struct Triangle triangles[2];
    triangles[0].vertices[0] = get_vertex_id(width, coords[0]);
    triangles[0].vertices[1] = get_vertex_id(width, coords[1]);
    triangles[0].vertices[2] = get_vertex_id(width, coords[2]);

    triangles[1].vertices[0] = get_vertex_id(width, coords[1]);
    triangles[1].vertices[1] = get_vertex_id(width, coords[3]);
    triangles[1].vertices[2] = get_vertex_id(width, coords[2]);

    
    // TODO: calculate surface normal
}

__kernel void calculate_surface_normal(__global struct Vertex* out,
                                       image2d_t image)
{
    // TODO
}