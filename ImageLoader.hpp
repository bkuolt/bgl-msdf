




#include <vector>


#include <CL/opencl.hpp>

#include <spdlog/spdlog.h>


#include <filesystem>
#include <png++/png.hpp>


template<class Pixel>
constexpr int channels_of()
{
    if constexpr (std::is_same_v<Pixel, png::gray_pixel>)        return 1;
    if constexpr (std::is_same_v<Pixel, png::rgb_pixel>)         return 3;
    if constexpr (std::is_same_v<Pixel, png::rgba_pixel>)        return 4;
    return -1;
}







cl::Image2D LoadImage(cl::Context& context, const std::filesystem::path& path) {
    using PixelType = png::rgba_pixel;

    png::image<PixelType> png("test.png");
    auto pixelCount = png.get_width() * png.get_height() * channels_of<PixelType>();
    std::vector<uint8_t> pixels(pixelCount);

    const cl::ImageFormat imageFormat(CL_RGBA, CL_UNORM_INT8);

    return cl::Image2D(
        context,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        imageFormat,
        png.get_width(), png.get_height(),
        0,               // row_pitch: 0 => OpenCL nimmt width*pixelSize
        pixels.data()
    );

}

