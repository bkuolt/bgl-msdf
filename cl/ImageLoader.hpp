#ifndef IMAGELOADER_HPP
#define IMAGELOADER_HPP

#include <vector>

#include <CL/opencl.hpp>

#ifdef USE_PNG 
#include <png++/png.hpp>
#endif

#include <spdlog/spdlog.h>
#include <filesystem>


struct Image {
  std::vector<std::uint8_t> pixels;
  size_t width;
  size_t height;

  Image (const std::filesystem::path &path) {
#ifdef USE_PNG 
      png::image<png::rgba_pixel> png(path.string());

      width = png.get_width();
      height = png.get_height();

      const auto pixelCount {width * height * 4};
      pixels.resize(pixelCount);

      for (size_t y = 0, i = 0; y < height; ++y) {
          for (size_t x = 0; x < width; ++x) {
              const auto& pixel = png[y][x];
              pixels[i++] = pixel.red;
              pixels[i++] = pixel.green;
              pixels[i++] = pixel.blue;
              pixels[i++] = pixel.alpha;
          }
      }

#else 
      throw std::runtime_error("PNG support not enabled. Rebuild with USE_PNG defined.");
#endif  
    }
};


cl::Image2D LoadImage(cl::Context &context, const std::filesystem::path &path) {
  const Image png(path);
  const cl::ImageFormat imageFormat(CL_RGBA, CL_UNORM_INT8);
  cl::Image2D image(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                    imageFormat,
                    png.width, png.height,
                    0, 
                    (void*)png.pixels.data());

  spdlog::info("Loaded image '{}' ({}x{})", path.string(), png.width, png.height);
  return image;
}

#endif // IMAGELOADER_HPP
