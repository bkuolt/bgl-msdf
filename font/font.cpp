#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H

#include <hb-ft.h>
#include <hb.h>

#include "Font.hpp"
#include "details.hpp"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <filesystem>

std::vector<uint8_t> Render(FT_Face face, std::string_view utf8Text,
                            const glm::uvec2 &size);

int RunFreetype(int argc, char **argv) {

  const char *font_path =
      (argc >= 2) ? argv[1] : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
  const char *text = (argc >= 3) ? argv[2] : "Hello, BGL!";

  const auto directory = std::filesystem::path(argv[0]).parent_path();
  const std::filesystem::path path = directory / "out.pgm";

  char *fpp = details::fontconfig_find_font_file("sans:weight=bold");
  spdlog::info("found font file: {}", fpp);

  FT_Library ft = details::Initialize();
  FT_Face face = details::LoadFace(ft, font_path);

  const glm::uvec2 size{800, 200};
  auto image = Render(face, text, size);

  if (!details::save_pgm(path.string().c_str(), image.data(), size)) {
    return 2;
  }

  FT_Done_Face(face);
  FT_Done_FreeType(ft);
  return 0;
}

// -------------------------------------------------------------------------------------
glm::ivec2 RenderGlyphByIndex(FT_Face face, FT_UInt glyphIndex,
                              const glm::ivec2 &pen, uint8_t *img,
                              const glm::uvec2 &size);

std::vector<uint8_t> Render(FT_Face face, std::string_view utf8Text,
                            const glm::uvec2 &size) {
  std::vector<uint8_t> img((size_t)size.x * size.y, 0); // RAII, kein leak

  glm::ivec2 pen{50, 120};

  const auto cps = utf8Text; // Utf8ToCodepoints(utf8Text);

  FT_UInt prevGlyph = 0;
  const bool hasKerning = FT_HAS_KERNING(face);

  for (char32_t cp : cps) {
    // Hole Glyph explizit via Codepoint → GlyphIndex
    FT_UInt glyph = FT_Get_Char_Index(face, (FT_ULong)cp);

    if (glyph == 0) {
      // Fallback: '?' (wenn vorhanden)
      FT_UInt fallback = FT_Get_Char_Index(face, (FT_ULong)'?');
      if (fallback == 0)
        continue;
      glyph = fallback;
    }

    // Kerning (nur zwischen Glyphen, wenn Font das unterstützt)
    if (hasKerning && prevGlyph && glyph) {
      FT_Vector delta{};
      if (FT_Get_Kerning(face, prevGlyph, glyph, FT_KERNING_DEFAULT, &delta) ==
          0) {
        pen.x += (int)(delta.x >> 6);
        pen.y += (int)(delta.y >> 6);
      }
    }

    // Optional: early out wenn wir rechts raus sind
    if (pen.x >= (int)size.x)
      break;

    pen += RenderGlyphByIndex(face, glyph, pen, img.data(), size);
    prevGlyph = glyph;
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
