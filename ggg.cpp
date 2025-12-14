



#include <CL/cl2.hpp>
#include <vector>
#include <format>




//#include <spdlog>




int main() {


  

    auto device =cl::Device::getDefault();
    cl_platform_id pid = device.getInfo<CL_DEVICE_PLATFORM>();

    cl::Platform platform(pid);
    std::string pName  = platform.getInfo<CL_PLATFORM_NAME>();
    std::string pVer   = platform.getInfo<CL_PLATFORM_VERSION>();



#if 0
    // 1) Platform + Device auswählen
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty()) {
        std::cerr << "No OpenCL platforms found.\n";
        return 1;
    }

    cl::Platform platform = platforms.front();




        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (devices.empty()) {
            // fallback: CPU
            platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
        }
        if (devices.empty()) {
            std::cerr << "No OpenCL devices found (GPU/CPU).\n";
            return 1;
        }

        cl::Device device = devices.front();
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << "\n";



      cl::Device::






        // 2) Context + Queue
        cl::Context context(device);
        cl::CommandQueue queue(context, device);



    // 3) Programm bauen
        cl::Program program(context, cl::Program::Sources{
            {kKernelSrc, std::strlen(kKernelSrc)}
        });

        try {
            program.build({device});
        } catch (...) {
            // Build-Log ausgeben
            std::cerr << "Build log:\n"
                      << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
            throw;
        }

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

        // 7) Download + Check
        queue.enqueueReadBuffer(bufOut, CL_TRUE, 0, sizeof(float) * N, out.data());

        // quick verify
        bool ok = true;
        for (int i = 0; i < N; ++i) {
            float ref = a[i] + b[i];
            if (out[i] != ref) { ok = false; break; }
        }
        std::cout << (ok ? "OK" : "Mismatch") << "\n";
        std::cout << "out[0]=" << out[0] << " out[10]=" << out[10] << "\n";

        return ok ? 0 : 2;
    }
    catch (const cl::Error& e) {
        std::cerr << "OpenCL error: " << e.what() << " (" << e.err() << ")\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }


#endif










}