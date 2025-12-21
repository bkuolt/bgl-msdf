


#include <CL/opencl.hpp>
#include <spdlog/spdlog.h>
#include <format>




void printDeviceInfo(const cl::Device& device) {
    std::string name = device.getInfo<CL_DEVICE_NAME>();
    std::string vendor = device.getInfo<CL_DEVICE_VENDOR>();
    std::string version = device.getInfo<CL_DEVICE_VERSION>();
    std::string driverVersion = device.getInfo<CL_DRIVER_VERSION>();
    size_t globalMemSize = device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
    size_t localMemSize = device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
    size_t maxComputeUnits = device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();

    spdlog::info("Device Name: {}", name);
    spdlog::info("Vendor: {}", vendor);
    spdlog::info("Version: {}", version);
    spdlog::info("Driver Version: {}", driverVersion);
    spdlog::info("Global Memory Size: {} MB", globalMemSize / (1024 * 1024));
    spdlog::info("Local Memory Size: {} KB", localMemSize / 1024);
    spdlog::info("Max Compute Units: {}", maxComputeUnits);
};  //



void PrintPlatformInfo(const cl::Platform& platform) {
    std::string pName  = platform.getInfo<CL_PLATFORM_NAME>();
    std::string pVer   = platform.getInfo<CL_PLATFORM_VERSION>();
    std::string pVendor = platform.getInfo<CL_PLATFORM_VENDOR>();
    std::string pProfile = platform.getInfo<CL_PLATFORM_PROFILE>();
    std::string pExtensions = platform.getInfo<CL_PLATFORM_EXTENSIONS>();


    spdlog::info("Platform: {} ({})", pName, pVer);
    spdlog::info(" Vendor: {}", pVendor);
    spdlog::info(" Profile: {}", pProfile);
    spdlog::info(" Extensions: {}", pExtensions);
}  //

