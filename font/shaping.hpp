#ifndef FONT_SHAPING_HPP
#define FONT_SHAPING_HPP

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Glyph {
  FT_UInt glyphIndex;
  glm::ivec2 offset;
  glm::ivec2 advance;
};

struct GlyphRun {
  std::vector<Glyph> glyphs;
  glm::uvec2 size;
};

GlyphRun shape(FT_Face face, std::string_view utf8Text);

#endif // FONT_SHAPING_HPP