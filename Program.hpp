
#include <vector>
#include <format>


#include <CL/opencl.hpp>

#include <spdlog/spdlog.h>
#include <numeric>
#include <filesystem>
#include <fstream>


////////////////////////////



class Program {
    public:
        Program(cl::Context& context) : _context(context) { }

        cl::Program build(cl::Device& device, const std::filesystem::path& path) {
            const cl::Program::Sources sources { loadSource(path) };
            _program = cl::Program(_context, sources);
            _program.build({device});
            spdlog::info("OpenCL Program built successfully.");

            // TODO: cache binary
            return _program;
        }

        cl::Kernel getKernel(const std::string& name) {
            cl::Kernel kernel(_program, name.c_str());
            return kernel;
        }

     private:
       cl::Program::Sources loadSource(const std::filesystem::path& path) {
            std::ifstream file(path);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open kernel source file: " + path.string());
            }   

            const std::string source {std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>())};
            return {source};
        }

    private:
        cl::Context& _context;
        cl::Program _program;
};




