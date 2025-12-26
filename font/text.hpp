#ifndef FONT_FONT_HPP
#define FONT_FONT_HPP

#include <filesystem>
#include <string_view>
#include <glm/vec2.hpp>

void RunFreetype(const std::filesystem::path &imagePath, std::string_view text);

// TODO:
class Font {};
class FontFace {};

class Bitmap {};

class GlyphCache {
  struct key_type {
    FontFace face;
    uint size;
    uint32_t codepoint;
  };
};

class Glyph {
  Bitmap bitmap;
  glm::ivec2 bearing; 
  glm::ivec2 advance;
};

class TextRenderer {
public:
  TextRenderer();
  ~TextRenderer();

  Bitmap render(FontFace face, int size, const std::string_view text);

private:
  Glyph renderGlyph(FontFace face, int size, uint32_t codepoint);
  void cacheGlyph(Glyph glyph, uint size, uint32_t codepoint);

  GlyphCache glyphCache;
};

#endif // FONT_FONT_HPP