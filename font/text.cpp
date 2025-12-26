#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <format>
#include <stdexcept>
#include <string>

#include <exception>
#include <filesystem>

#include "render.hpp"
#include "shaping.hpp"

std::string find_font(const std::string &query);

namespace {

/* ------------------------------------------------------------------------- */

FT_Library Initialize() {
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    throw std::runtime_error("FT_Init_FreeType failed");
  }
  return ft;
}

FT_Face LoadFace(FT_Library ft, std::string fontPath, int size = 64) {
  FT_Face face;

  if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
    throw new std::runtime_error(
        std::format("FT_New_Face failed (font: {})", fontPath));
  }

  // Set font size in pixels
  if (FT_Set_Pixel_Sizes(face, 0, size)) {
    FT_Done_Face(face);
    throw std::runtime_error("FT_Set_Pixel_Sizes failed");
  }

  spdlog::info("Loaded {}", fontPath);
  return face;
}

} // namespace

void RunFreetype(const std::filesystem::path &imagePath,
                 std::string_view text) {
  const std::string font_path = find_font("sans:weight=bold");

  FT_Library ft;
  FT_Face face;

  try {
    ft = Initialize();
    face = LoadFace(ft, font_path);
    spdlog::info("initialize FreeType");

    GlyphRun run = shape(face, text);
    const auto image{Render(face, run)};

    save_pgm(imagePath, image, run.size);
    spdlog::info("rendered text to image");

  } catch (const std::exception &e) {
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    throw;
  }

  FT_Done_Face(face);
  FT_Done_FreeType(ft);
  spdlog::info("shutdown FreeType");
}
