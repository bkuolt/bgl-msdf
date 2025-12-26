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

struct Glyph {
  FT_UInt glyphIndex;
  glm::ivec2 offset;
  glm::ivec2 advance;
};

struct GlyphRun {
  std::vector<Glyph> glyphs;
  glm::uvec2 size;
};

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
struct RectI {
  glm::ivec2 min{0, 0};
  glm::ivec2 max{0, 0};
};

static inline int floor26(int32_t v) { return (int)(v >> 6); }
static inline int ceil26(int32_t v) { return (int)((v + 63) >> 6); }

RectI CalculateBoundingRectPx(hb_font_t *hbFont, hb_buffer_t *buf) {
  unsigned count = 0;
  auto *infos = hb_buffer_get_glyph_infos(buf, &count);
  auto *pos = hb_buffer_get_glyph_positions(buf, &count);

  RectI r;
  r.min = {INT32_MAX, INT32_MAX};
  r.max = {INT32_MIN, INT32_MIN};

  int32_t x = 0, y = 0;
  bool any = false;

  for (unsigned i = 0; i < count; ++i) {
    hb_glyph_extents_t ext{};
    if (!hb_font_get_glyph_extents(hbFont, infos[i].codepoint, &ext)) {
      x += pos[i].x_advance;
      y += pos[i].y_advance;
      continue;
    }

    // hb units (26.6)
    int32_t gx = x + pos[i].x_offset + ext.x_bearing;
    int32_t gy = y + pos[i].y_offset + ext.y_bearing;

    int32_t x0 = gx;
    int32_t x1 = gx + ext.width;

    // HB: y up, height often negative
    int32_t yTop = gy;
    int32_t yBot = gy + ext.height;

    int32_t y0 = std::min(yTop, yBot);
    int32_t y1 = std::max(yTop, yBot);

    // Convert to pixels *per glyph* (safe rounding)
    int px0 = floor26(x0);
    int px1 = ceil26(x1);
    int py0 = floor26(y0);
    int py1 = ceil26(y1);

    r.min.x = std::min(r.min.x, px0);
    r.min.y = std::min(r.min.y, py0);
    r.max.x = std::max(r.max.x, px1);
    r.max.y = std::max(r.max.y, py1);

    any = true;

    x += pos[i].x_advance;
    y += pos[i].y_advance;
  }

  if (!any)
    return RectI{{0, 0}, {0, 0}};
  return r;
}

static inline RectI TranslateRect(const RectI &r, const glm::ivec2 &pen) {
  return RectI{r.min + pen, r.max + pen};
}

static inline glm::uvec2 SizeOfRect(const RectI &r) {
  glm::ivec2 s = r.max - r.min;
  return glm::uvec2((unsigned)std::max(0, s.x), (unsigned)std::max(0, s.y));
}

// What you asked for: compute required image size given "bbox without pen" +
// pen
static inline glm::uvec2 RequiredImageSize(const RectI &bboxWithoutPen,
                                           const glm::ivec2 &pen,
                                           int padding = 0) {
  RectI r = TranslateRect(bboxWithoutPen, pen);

  // If you want the image to start at (0,0), clamp to that.
  // Otherwise negative mins would be outside the image.
  r.min.x = std::min(r.min.x, 0);
  r.min.y = std::min(r.min.y, 0);
  r.max.x = std::max(r.max.x, 0);
  r.max.y = std::max(r.max.y, 0);

  glm::ivec2 size = (r.max - r.min) + glm::ivec2(padding * 2);
  return glm::uvec2((unsigned)std::max(0, size.x),
                    (unsigned)std::max(0, size.y));
}

GlyphRun CreateGlyphRun(FT_Face face, std::string_view utf8Text) {
  hb_font_t *hb_font = hb_ft_font_create_referenced(face);
  hb_buffer_t *buf = hb_buffer_create();

  hb_buffer_add_utf8(buf, utf8Text.data(), (int)utf8Text.size(), 0,
                     (int)utf8Text.size());
  hb_buffer_guess_segment_properties(buf);

  // Shaping: macht aus Text -> Glyph IDs + Positioning
  hb_shape(hb_font, buf, nullptr, 0);

  unsigned int count = 0;
  hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buf, &count);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, &count);
  spdlog::info("glyph count: {}", count);

  // I want to store the glyphs and their positions in a struct
  std::vector<Glyph> glyphs;
  glyphs.reserve(count);

  for (unsigned int i = 0; i < count; ++i) {
    Glyph g;
    g.glyphIndex = infos[i].codepoint;
    g.offset = {pos[i].x_offset >> 6, pos[i].y_offset >> 6}; // from 26.6 to int
    g.advance = {pos[i].x_advance >> 6, pos[i].y_advance >> 6};

    glyphs.push_back(g);
  }

  GlyphRun run;
  run.glyphs = std::move(glyphs);

  auto boundingRect = CalculateBoundingRectPx(hb_font, buf);

  const int padding = 0;
  glm::ivec2 pen(
      50,
      0); //= glm::ivec2(padding) - boundingRect.min; // bbox.min is without pen
  pen.x = 0;
  pen.y = (face->size->metrics.ascender + 63) >> 6; // baseline in px

  glm::uvec2 imgSize = RequiredImageSize(boundingRect, pen, padding);
  spdlog::info("Bounding size: ({}, {})", imgSize.x, imgSize.y);

  run.size = imgSize;

  hb_buffer_destroy(buf);
  hb_font_destroy(hb_font);

  return run;
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