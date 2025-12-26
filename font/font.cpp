#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H

#include <fontconfig/fontconfig.h>

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <format>
#include <stdexcept>
#include <string>

#include <exception>
#include <filesystem>

#include "render.hpp"

namespace {

// Returns a malloc()'d UTF-8 path to the matched font file, or NULL on failure.
// Caller must free() the returned string.
char *fontconfig_find_font_file(const char *query) {
  if (!query)
    return NULL;

  // Initialize fontconfig (safe to call multiple times; returns FcFalse on
  // error).
  if (!FcInit()) {
    return NULL;
  }

  FcPattern *pat = FcNameParse((const FcChar8 *)query);
  if (!pat)
    return NULL;

  // Apply config substitutions (aliases, defaults, etc.)
  FcConfigSubstitute(NULL, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  FcResult result = FcResultNoMatch;
  FcPattern *match = FcFontMatch(NULL, pat, &result);

  FcChar8 *file = NULL;
  char *out = NULL;

  if (match && FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch &&
      file) {
    // Copy to a normal malloc() buffer so caller can free() easily.
    size_t n = strlen((const char *)file);
    out = (char *)malloc(n + 1);
    if (out) {
      memcpy(out, (const char *)file, n + 1);
    }
  }

  if (match)
    FcPatternDestroy(match);
  FcPatternDestroy(pat);

  return out;
}

std::string find_font(const std::string &query) {

  char *path = fontconfig_find_font_file(query.c_str());
  if (!path) {
    throw std::runtime_error(
        std::format("could not find font for query '{}'", query));
  }

  spdlog::info("found font file: {}", font_path);
  return std::string(path);
}

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

    const glm::uvec2 size{800, 200};
    const auto image{Render(face, text, size)};
    save_pgm(imagePath, image, size);
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
