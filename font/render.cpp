#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H

#include <hb-ft.h>
#include <hb.h>

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <string>

#include <filesystem>
#include <fmt/core.h>
#include <vector>

glm::ivec2 RenderGlyphByIndex(FT_Face face, FT_UInt glyphIndex,
                              const glm::ivec2 &pen, uint8_t *img,
                              const glm::uvec2 &size);

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

void apply_kerning(FT_Face face, FT_UInt prevGlyph, FT_UInt glyph,
                   glm::ivec2 &pen) {
  // Kerning (nur zwischen Glyphen, wenn Font das unterstützt)
  if (prevGlyph && glyph) {
    FT_Vector delta{};
    if (FT_Get_Kerning(face, prevGlyph, glyph, FT_KERNING_DEFAULT, &delta) ==
        0) {
      pen.x += (int)(delta.x >> 6);
      pen.y += (int)(delta.y >> 6);
    }
  }
}

std::vector<uint8_t> Render(FT_Face face, std::string_view utf8Text,
                            const glm::uvec2 &size) {

  std::vector<uint8_t> img((size_t)size.x * size.y, 0);
  const bool hasKerning = FT_HAS_KERNING(face);

  FT_UInt prevGlyph = 0;
  glm::ivec2 pen{50, 120};

  for (char32_t cp : utf8Text) {
    FT_UInt glyph = get_glyph(face, cp);

    if (hasKerning) {
      apply_kerning(face, prevGlyph, glyph, pen);
    }

    if (pen.x >= (int)size.x) {
      spdlog::info("reached end of line, stopping rendering.");
      break;
    }

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

/***************************************************************** */

int RenderHB(FT_Face face, std::string_view utf8Text) {

  glm::ivec2 pen{50, 120};

  hb_font_t *hb_font = hb_ft_font_create_referenced(face);
  hb_buffer_t *buf = hb_buffer_create();

  hb_buffer_add_utf8(buf, utf8Text.data(), (int)utf8Text.size(), 0,
                     (int)utf8Text.size());

  // Auto: erkennt Script/Direction oft selbst; für safety könntest du setzen:
  hb_buffer_guess_segment_properties(buf);

  // Shaping: macht aus Text -> Glyph IDs + Positioning
  hb_shape(hb_font, buf, nullptr, 0);

  unsigned int count = 0;
  hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buf, &count);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, &count);

  spdlog::info("glyphs: {}", count);

  // HarfBuzz arbeitet in 26.6 fixed point (wie FreeType advances)
  for (unsigned i = 0; i < count; ++i) {

    const FT_UInt glyphIndex =
        infos[i].codepoint; // das ist jetzt die Glyph-ID!
    const int x_off = pos[i].x_offset >> 6;
    const int y_off = pos[i].y_offset >> 6;

    glm::ivec2 drawPen =
        pen + glm::ivec2{x_off, -y_off}; // y-Achse beachten (Baseline)
#if 0
        pen += RenderGlyphByIndex(face, glyphIndex, drawPen, img.data(), W, H);
#endif
    // advance NICHT aus FreeType nehmen, sondern aus HarfBuzz!
    pen.x += (pos[i].x_advance >> 6);
    pen.y -= (pos[i].y_advance >> 6);

#if 0
        if (pen.x >= W) break;
#endif
  }

  hb_buffer_destroy(buf);
  hb_font_destroy(hb_font);

  return 0;
}

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