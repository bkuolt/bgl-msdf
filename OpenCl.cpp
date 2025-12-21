



#include <vector>
#include <format>


#include <CL/opencl.hpp>

#include <spdlog/spdlog.h>
#include <numeric>
#include <filesystem>
#include <fstream>

#include "Device.hpp"
#include "ImageLoader.hpp"
#include "Program.hpp"




int RunOpenCL() {

    auto device { cl::Device::getDefault() };
    cl::Platform platform { device.getInfo<CL_DEVICE_PLATFORM>() };
    PrintPlatformInfo(platform);

    // 2) Context + Queue
    cl::Context context(device);
    cl::CommandQueue queue(context, device);

   // 3) Programm bauen
    Program programBuilder(context);
    cl::Program program = programBuilder.build(device, "/home/bastian/Desktop/bgl-cl/vadd.cl");


    try { 
        cl::Kernel kernel = programBuilder.getKernel("vadd");

   
        // 4) Daten vorbereiten
        constexpr int N = 1024;
        std::vector<float> a(N), b(N), out(N, 0.0f);
        std::iota(a.begin(), a.end(), 0.0f);      // 0..N-1
        std::iota(b.begin(), b.end(), 1000.0f);   // 1000..1000+N-1

        // 5) Buffers + Upload
        cl::Buffer bufA(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR,sizeof(float) * N, a.data());
        cl::Buffer bufB(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR,sizeof(float) * N, b.data());
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