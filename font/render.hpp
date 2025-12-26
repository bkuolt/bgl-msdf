#ifndef FONT_RENDER_HPP
#define FONT_RENDER_HPP

#include "shaping.hpp" // GlyphRun

#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

void save_pgm(const std::filesystem::path &path, std::vector<uint8_t> image,
              const glm::uvec2 &size);

std::vector<uint8_t> Render(FT_Face face, GlyphRun run);

glm::ivec2 CalculateBoundingRect(FT_Face face, std::string_view utf8Text);

#endif // FONT_RENDER_HPP