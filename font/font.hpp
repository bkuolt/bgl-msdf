#ifndef FONT_FONT_HPP
#define FONT_FONT_HPP

#include <filesystem>
#include <string_view>

void RunFreetype(const std::filesystem::path &imagePath, std::string_view text);

// TODO:
class Font {};
class FontFace {};

class Bitmap {};
class GlyphCache {};

class Glyph {
  Bitmap bitmap;
  int size;
};

class TextRenderer {
public:
  TextRenderer();
  ~TextRenderer();

  Bitmap render(FontFace face, int size, const std::string_view text);

private:
  Glyph renderGlyph(FontFace face, int size, uint32_t codepoint);
  void cacheGlyph(Glyph glyph, uint32_t codepoint);

  GlyphCache glyphCache;
};

#endif // FONT_FONT_HPP