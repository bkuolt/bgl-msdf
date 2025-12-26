
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

void save_pgm(const std::filesystem::path &path, std::vector<uint8_t> image,
              const glm::uvec2 &size);

std::vector<uint8_t> Render(FT_Face face, std::string_view utf8Text,
                            const glm::uvec2 &size);