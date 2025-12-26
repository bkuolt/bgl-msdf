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

namespace details {

FT_Library Initialize() {
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    throw std::runtime_error("FT_Init_FreeType failed");
  }
  return ft;
}

FT_Face LoadFace(FT_Library ft, const char *fontPath, int size = 64) {
  FT_Face face;

  if (FT_New_Face(ft, fontPath, 0, &face)) {
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

/** --------------------------------------------------------- */

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

static int save_pgm(const char *path, const unsigned char *img,
                    const glm::uvec2 &size) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return 0;
  fprintf(f, "P5\n%d %d\n255\n", size.x, size.y);
  fwrite(img, 1, (size_t)(size.x * size.y), f);
  fclose(f);

  spdlog::info("Saved image '{}'", path);
  return 1;
}

/** --------------------------------------------------------- */

} // namespace details