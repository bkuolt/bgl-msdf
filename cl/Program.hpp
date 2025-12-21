#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <CL/opencl.hpp>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>

/**
 * @brief Helps to build and manage OpenCL programs.
 */
class Program {
public:
  Program(cl::Context &context) : _context(context) {}

  cl::Program build(cl::Device &device, const std::filesystem::path &path) {
    spdlog::info("Building OpenCL Program from source: {}", path.string());
    
    const cl::Program::Sources sources{loadSource(path)};
    _program = cl::Program(_context, sources);
    _program.build({device});

    // Build-Log ausgeben
    spdlog::error("Build log:\n{}\n", _program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));

    spdlog::info("OpenCL Program built successfully.");

    // TODO: cache binary
    return _program;
  }

  cl::Kernel getKernel(const std::string &name) {
    cl::Kernel kernel(_program, name.c_str());
    return kernel;
  }

private:
  void load() {
    // TODO: load source
    // TODO: load binary
    // TODO: load SpirV
  }

  void loadBinary() {
    // TODO
  }

  void loadSpirV() {
    // TODO
  }

  cl::Program::Sources loadSource(const std::filesystem::path &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open kernel source file: " +
                               path.string());
    }

    const std::string source{std::string(std::istreambuf_iterator<char>(file),
                                         std::istreambuf_iterator<char>())};
    return {source};
  }


  void saveBinary() {
#if 0
    // 1) Wie groß ist das Binary?
    auto sizes = program.getInfo<CL_PROGRAM_BINARY_SIZES>();
    if (sizes.empty() || sizes[0] == 0) {
        throw std::runtime_error("No program binary available.");
    }

    // 2) Speicher allokieren & Binary holen
    std::vector<unsigned char> bin(sizes[0]);
    std::vector<unsigned char*> ptrs{ bin.data() };
    program.getInfo(CL_PROGRAM_BINARIES, &ptrs);

    // 3) Schreiben
    std::ofstream f(outFile, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to write: " + outFile.string());
    f.write(reinterpret_cast<const char*>(bin.data()), (std::streamsize)bin.size());
#endif
  }

private:
  cl::Context &_context;
  cl::Program _program;
};

class ProgramCache {
    // TODO: implement cache mechanism
public:
    ProgramCache() {
      auto home = std::getenv("HOME"); // just to set a breakpoint here
      spdlog::info("HOME={}", home ? home : "not set");

    }
};

#endif // PROGRAM_HPP