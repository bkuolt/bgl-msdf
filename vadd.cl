      __kernel void vadd(
            __global const float* a,
            __global const float* b,
            __global float* out)
        {
            int gid = get_global_id(0);
            out[gid] = a[gid] + b[gid];
        }