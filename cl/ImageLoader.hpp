#ifndef IMAGELOADER_HPP
#define IMAGELOADER_HPP

#include <vector>

#include <CL/opencl.hpp>
#include <png++/png.hpp>

#include <spdlog/spdlog.h>
#include <filesystem>


struct Image {
  std::vector<std::uint8_t> pixels;
  size_t width;
  size_t height;

  Image (const std::filesystem::path &path) {
      png::image<png::rgba_pixel> png(path.string());

      const auto pixelCount {png.get_width() * png.get_height() * 4};
      pixels.resize(pixelCount);

      for (size_t y = 0, i = 0; y < png.get_height(); ++y) {
          for (size_t x = 0; x < png.get_width(); ++x) {
              const auto& pixel = png[y][x];
              pixels[i++] = pixel.red;
              pixels[i++] = pixel.green;
              pixels[i++] = pixel.blue;
              pixels[i++] = pixel.alpha;
          }
      }
    }
};


cl::Image2D LoadImage(cl::Context &context, const std::filesystem::path &path) {
  const Image image(path);
  const cl::ImageFormat imageFormat(CL_RGBA, CL_UNORM_INT8);
  return cl::Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                    imageFormat,
                    image.width, image.height,
                    0, 
                    (void*)image.pixels.data());
}

#endif // IMAGELOADER_HPP
