#include <CL/opencl.hpp>

#include <spdlog/spdlog.h>
#include <vector>

#include "Device.hpp"
#include "Image.hpp"
#include "Program.hpp"
#include <glm/glm.hpp>

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
};

struct Triangle {
  uint32_t vertices[3];
};

template <typename T> cl::Buffer make_buffer(cl::Context &context, int size) {
  return cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                    sizeof(T) * size, nullptr);
}

template <typename T>
cl::Buffer make_buffer(cl::Context &context, std::vector<T> data) {
  return cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                    sizeof(T) * data.size(), data.data());
}

int RunOpenCL(int argc, char **argv) {
  const auto assetsDir =
      std::filesystem::path(argv[0]).parent_path() / "assets";
  const std::filesystem::path kernelPath{assetsDir / "vadd.cl"};
  const std::filesystem::path imagePath{assetsDir / "heightmap.png"};
  // TODO: check files

  // setup OpenCL
  auto device{GetOpenCLDevice()};
  cl::Context context(device);
  cl::CommandQueue queue(context, device);

  // build OpenCL program
  Program programBuilder(context);
  cl::Program program = programBuilder.build(device, kernelPath);

  try {
    cl::Kernel kernel = programBuilder.getKernel("calculate_geometry");

    // load heightmap image
    cl::Image2D heightmap = LoadImage(context, imagePath);
    auto width = heightmap.getImageInfo<CL_IMAGE_WIDTH>();
    auto height = heightmap.getImageInfo<CL_IMAGE_HEIGHT>();
    spdlog::info("uploaded cl::Image2D ({}x{})", width, height);

    // 4) Prepare Data
    const auto numTriangles{width * height * 2};
    const size_t numVertices{numTriangles * 3};

    const size_t N = numVertices * 3 * 4; // x,y,z pro Vertex
    std::vector<float> a(N), b(N), out(N, 0.0f);

    // 5) Buffers + Upload
    spdlog::info("creating buffers and uploading ({} MB)",
                 (sizeof(float) * N * 3) / (1024 * 1024));

    cl::Buffer bufA = make_buffer(context, a);
    cl::Buffer triangles = make_buffer(context, b);
    cl::Buffer vertices = make_buffer<float>(context, N);
    spdlog::info("uploaded buffers");

    spdlog::info("vertices: {}, triangles: {}", numVertices, numTriangles);

    // 6) Kernel Args + Dispatch
    kernel.setArg(0, vertices);
    kernel.setArg(1, triangles);
    kernel.setArg(2, vertices);
    kernel.setArg(3, heightmap);

    spdlog::info("kernel dispatched");
    queue.enqueueNDRangeKernel(kernel, cl::NullRange,
                               cl::NDRange(width, height), cl::NullRange);

    spdlog::info("Waiting to finish...");
    queue.finish();
    spdlog::info("Done");

    // 7) Download
    queue.enqueueReadBuffer(vertices, CL_TRUE, 0, sizeof(float) * N,
                            out.data());
  } catch (const std::exception &e) {
    spdlog::error("Exception: {}", e.what());
    return 1;
  }

  return 0;
}