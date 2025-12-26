#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H

#include "shaping.hpp"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <string>

#include <filesystem>
#include <fmt/core.h>
#include <vector>

glm::ivec2 RenderGlyphByIndex(FT_Face face, FT_UInt glyphIndex,
                              const glm::ivec2 &pen, uint8_t *img,
                              const glm::uvec2 &size);

glm::ivec2 CalculateBoundingRect(FT_Face face, std::string_view utf8Text);
GlyphRun CreateGlyphRun(FT_Face face, std::string_view utf8Text);

/**--------------------------------------------------------------------------------------------------
 */

FT_UInt get_glyph(FT_Face face, FT_ULong cp) {
  FT_UInt glyph = FT_Get_Char_Index(face, cp);

  if (glyph == 0) {
    FT_UInt fallback = FT_Get_Char_Index(
        face, (FT_ULong)'?'); // Fallback: '?' (wenn vorhanden)
    if (fallback == 0) {
      throw std::runtime_error(fmt::format(
          "No glyph for codepoint U+{:04X} and no fallback '?'", cp));
    }

    glyph = fallback;
  }

  return glyph;
}

std::vector<uint8_t> Render(FT_Face face, GlyphRun run) {
  auto glyphs = run.glyphs;

  std::vector<uint8_t> img((size_t)run.size.x * run.size.y, 0);

  glm::ivec2 pen;
  pen.x = 0;                                        // 50;
  pen.y = (face->size->metrics.ascender + 63) >> 6; // baseline in px

  for (const Glyph &g : glyphs) {
#if 0
    if (pen.x >= (int)size.x) {
      spdlog::info("reached end of line, stopping rendering.");
      break;
    }
#endif

    spdlog::trace("glyph index: {}, offset: ({},{}), advance: ({},{})",
                  g.glyphIndex, g.offset.x, g.offset.y, g.advance.x,
                  g.advance.y);

    // apply HarfBuzz offsets (positions are in pixels already)
    const glm::ivec2 place = pen + g.offset;
    // render the glyph at the positioned pen
    RenderGlyphByIndex(face, g.glyphIndex, place, img.data(), run.size);

    // advance pen by the shaped advance (use HarfBuzz-provided advance)
    pen += g.advance;
  }

  return img;
}

// -------------------------------------------------------------------------------------

// Blend a FreeType 8-bit coverage bitmap onto an 8-bit grayscale image (white
// text on black).
static void blend_glyph_bitmap(unsigned char *img, int w, int h,
                               const FT_Bitmap *bm, int x0, int y0) {
  // FreeType bitmap buffer contains coverage values 0..255 (for
  // FT_PIXEL_MODE_GRAY)
  for (int row = 0; row < (int)bm->rows; ++row) {
    for (int col = 0; col < (int)bm->width; ++col) {
      int x = x0 + col;
      int y = y0 + row;

      unsigned char a = bm->buffer[row * bm->pitch + col]; // coverage
      if (a == 0)
        continue;

      // simple "over" blend on grayscale: dst = dst + a*(255-dst)/255
      if (x < 0 || y < 0 || x >= w || y >= h)
        continue;
      unsigned char dst = img[y * w + x];
      unsigned int out = dst + (a * (255u - dst)) / 255u;
      img[y * w + x] = (unsigned char)out;
    }
  }
}

glm::ivec2 RenderGlyphByIndex(FT_Face face, FT_UInt glyphIndex,
                              const glm::ivec2 &pen, uint8_t *img,
                              const glm::uvec2 &size) {
  if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT)) {
    throw std::runtime_error(
        std::format("FT_Load_Glyph failed (glyphIndex={})", glyphIndex));
  }

  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
    throw std::runtime_error("FT_Render_Glyph failed");
  }

  FT_GlyphSlot g = face->glyph;

  const int x0 = pen.x + g->bitmap_left;
  const int y0 = pen.y - g->bitmap_top;
  // spdlog::info("placed glypth at ({},{})", x0, y0);

  if (g->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
    // WICHTIG: blend_glyph_bitmap MUSS clippen (x0/y0 und bitmap bounds)
    blend_glyph_bitmap(img, size.x, size.y, &g->bitmap, x0, y0);
  } else {
    throw std::runtime_error(
        std::format("Unsupported pixel mode {}", (int)g->bitmap.pixel_mode));
  }

  return {(int)(g->advance.x >> 6), (int)(g->advance.y >> 6)};
}

/***************************************************************** */

void save_pgm(const std::filesystem::path &path, std::vector<uint8_t> image,
              const glm::uvec2 &size) {
  FILE *f = fopen(path.string().c_str(), "wb");
  if (!f)
    throw std::runtime_error("");

  fprintf(f, "P5\n%d %d\n255\n", size.x, size.y);
  fwrite(image.data(), 1, (size_t)(size.x * size.y), f);
  fclose(f);

  spdlog::info("Saved image '{}'", path.string());
}