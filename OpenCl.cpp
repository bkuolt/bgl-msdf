



#include <vector>
#include <format>


#include <CL/opencl.hpp>

#include <spdlog/spdlog.h>
#include <numeric>




int RunOpenCL() {


  

    auto device =cl::Device::getDefault();

    cl::Platform platform = device.getInfo<CL_DEVICE_PLATFORM>();


    std::string pName  = platform.getInfo<CL_PLATFORM_NAME>();
    std::string pVer   = platform.getInfo<CL_PLATFORM_VERSION>();
    // TODO print more info



    std::string pVendor = platform.getInfo<CL_PLATFORM_VENDOR>();
    std::string pProfile = platform.getInfo<CL_PLATFORM_PROFILE>();
    std::string pExtensions = platform.getInfo<CL_PLATFORM_EXTENSIONS>();



    spdlog::info("Platform: {} ({})", pName, pVer);
    spdlog::info(" Vendor: {}", pVendor);
    spdlog::info(" Profile: {}", pProfile);
    spdlog::info(" Extensions: {}", pExtensions);





    // 2) Context + Queue
    cl::Context context(device);
    cl::CommandQueue queue(context, device);

    // 3) Programm bauen

    const char* kKernelSrc = R"(
        __kernel void vadd(
            __global const float* a,
            __global const float* b,
            __global float* out)
        {
            int gid = get_global_id(0);
            out[gid] = a[gid] + b[gid];
        }
    )";
    cl::Program program(context, cl::Program::Sources{
        {kKernelSrc, std::strlen(kKernelSrc)}
    });

    try {
        program.build({device});


        cl::Kernel kernel(program, "vadd");

        // 4) Daten vorbereiten
        constexpr int N = 1024;
        std::vector<float> a(N), b(N), out(N, 0.0f);

        std::iota(a.begin(), a.end(), 0.0f);      // 0..N-1
        std::iota(b.begin(), b.end(), 1000.0f);   // 1000..1000+N-1

        // 5) Buffers + Upload
        cl::Buffer bufA(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR,
                        sizeof(float) * N, a.data());
        cl::Buffer bufB(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR,
                        sizeof(float) * N, b.data());
        cl::Buffer bufOut(context, CL_MEM_WRITE_ONLY, sizeof(float) * N);

        // 6) Kernel Args + Dispatch
        kernel.setArg(0, bufA);
        kernel.setArg(1, bufB);
        kernel.setArg(2, bufOut);

        queue.enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(N),
            cl::NullRange
        );
        queue.finish();




        // 7) Download + Check
        queue.enqueueReadBuffer(bufOut, CL_TRUE, 0, sizeof(float) * N, out.data());

        // quick verify
        bool ok = true;
        for (int i = 0; i < N; ++i) {
            float ref = a[i] + b[i];
            if (out[i] != ref) { ok = false; break; }
        spdlog::info("{}", (ok ? "OK" : "Mismatch"));
        spdlog::info("{}", (ok ? "OK" : "Mismatch"));
        spdlog::info("out[0]={}, out[10]={}", out[0], out[10]);    

        }

    }
    catch (const std::exception& e) {
            spdlog::error("Exception: {}", e.what());
            return 1;
    }
    catch (...) {
        // Build-Log ausgeben
        spdlog::error("Build log:\n{}\n", program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
        throw;
    }














    return 0;





}