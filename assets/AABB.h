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