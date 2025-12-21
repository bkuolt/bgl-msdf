
#include <CL/opencl.hpp>

#include <spdlog/spdlog.h>
#include <vector>
#include <numeric>

#include "Device.hpp"
#include "ImageLoader.hpp"
#include "Program.hpp"
#include <glm/glm.hpp>


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};




int RunOpenCL() {

    auto device { GetOpenCLDevice() };

    // 2) Context + Queue
    cl::Context context(device);
    cl::CommandQueue queue(context, device);


   // 3) Programm bauen
    Program programBuilder(context);
    cl::Program program = programBuilder.build(device, "/home/bastian/Desktop/bgl-cl/vadd.cl");


    try { 
        cl::Kernel kernel = programBuilder.getKernel("vadd");

        cl::Image2D image = LoadImage(context, "/home/bastian/Desktop/bgl-cl/heightmap.png");
        auto width = image.getImageInfo<CL_IMAGE_WIDTH>();
        auto height = image.getImageInfo<CL_IMAGE_HEIGHT>();
        spdlog::info("uploaded cl::Image2D ({}x{})", width, height);






        // 4) Daten vorbereiten
        const auto numTriangles { width * height * 2 };
        const size_t numVertices { numTriangles * 3 };



        const size_t N = numVertices * 3 *4; // x,y,z pro Vertex
        std::vector<float> a(N), b(N), out(N, 0.0f);

        // 5) Buffers + Upload
        cl::Buffer bufA(context, CL_MEM_READ_WRITE  | CL_MEM_COPY_HOST_PTR,sizeof(float) * N, a.data());
        cl::Buffer bufB(context, CL_MEM_READ_WRITE  | CL_MEM_COPY_HOST_PTR,sizeof(float) * N, b.data());
        cl::Buffer bufOut(context, CL_MEM_READ_WRITE, sizeof(Vertex) * N);
        spdlog::info("uploaded buffers ({} MB)", (sizeof(float) * N * 3) / (1024 * 1024));


        spdlog::info("vertices: {}, triangles: {}", numVertices, numTriangles);


        // 6) Kernel Args + Dispatch
        kernel.setArg(0, bufA);
        kernel.setArg(1, bufB); 
        kernel.setArg(2, bufOut);

        kernel.setArg(3, image);

        spdlog::info("kernel dispatched");
        queue.enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(width, height),
            cl::NullRange
        );
        queue.finish();
        spdlog::info("Done");


        // 7) Download + Check
        queue.enqueueReadBuffer(bufOut, CL_TRUE, 0, sizeof(float) * N, out.data());

        // quick verify
        bool ok = true;
        for (int i = 0; i < N; ++i) {
            float ref = a[i] + b[i];
            if (out[i] != ref) { ok = false; break; }
      //  spdlog::info("{}", (ok ? "OK" : "Mismatch"));
      //  spdlog::info("{}", (ok ? "OK" : "Mismatch"));
      //  spdlog::info("out[0]={}, out[10]={}", out[0], out[10]);    

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