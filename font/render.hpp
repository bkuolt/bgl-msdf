
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

void save_pgm(const std::filesystem::path &path, std::vector<uint8_t> image,
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

GlyphRun CreateGlyphRun(FT_Face face, std::string_view utf8Text);

std::vector<uint8_t> Render(FT_Face face, GlyphRun run);
glm::ivec2 CalculateBoundingRect(FT_Face face, std::string_view utf8Text);