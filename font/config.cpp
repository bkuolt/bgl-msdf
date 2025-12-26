#include <cstdlib>
#include <cstring>
#include <fontconfig/fontconfig.h>
#include <format>
#include <spdlog/spdlog.h>
#include <string>

// Returns a malloc()'d UTF-8 path to the matched font file, or NULL on failure.
// Caller must free() the returned string.
static char *fontconfig_find_font_file(const char *query) {
  if (!query)
    return NULL;

  // Initialize fontconfig (safe to call multiple times; returns FcFalse on
  // error).
  if (!FcInit()) {
    return NULL;
  }

  FcPattern *pat = FcNameParse((const FcChar8 *)query);
  if (!pat)
    return NULL;

  // Apply config substitutions (aliases, defaults, etc.)
  FcConfigSubstitute(NULL, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  FcResult result = FcResultNoMatch;
  FcPattern *match = FcFontMatch(NULL, pat, &result);

  FcChar8 *file = NULL;
  char *out = NULL;

  if (match && FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch &&
      file) {
    // Copy to a normal malloc() buffer so caller can free() easily.
    size_t n = strlen((const char *)file);
    out = (char *)malloc(n + 1);
    if (out) {
      memcpy(out, (const char *)file, n + 1);
    }
  }

  if (match)
    FcPatternDestroy(match);
  FcPatternDestroy(pat);

  return out;
}

std::string find_font(const std::string &query) {

  char *path = fontconfig_find_font_file(query.c_str());
  if (!path) {
    throw std::runtime_error(
        std::format("could not find font for query '{}'", query));
  }

  spdlog::info("found font file: {}", path);
  return std::string(path);
}
