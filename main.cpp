#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exception>

#include <glm/glm.hpp>

// fontconfig_find_font_file.c
#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <hb.h>
#include <hb-ft.h>


#include "Font.hpp"

#include <spdlog/spdlog.h>


FT_Library Initialize() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        throw std::runtime_error("FT_Init_FreeType failed");
    }    
    return ft;
}

FT_Face LoadFace(FT_Library ft, const char* fontPath, int size = 64) {
    FT_Face face;

    if (FT_New_Face(ft, fontPath, 0, &face)) {
        throw new std::runtime_error(std::format("FT_New_Face failed (font: {})", fontPath));
    }

    // Set font size in pixels
    if (FT_Set_Pixel_Sizes(face, 0, size)) {
        FT_Done_Face(face);
        throw std::runtime_error("FT_Set_Pixel_Sizes failed");
    }

    spdlog::info("Loaded {}", fontPath);
    return face;
}



int Render(FT_Face face, std::string_view utf8Text);
char* fontconfig_find_font_file(const char* query);

int main(int argc, char** argv)
{
    const char* font_path = (argc >= 2) ? argv[1] : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    const char* text      = (argc >= 3) ? argv[2] : "Hello, BGL!";
   
 //Arial:style=Italic:pixelsize=32
  char* fpp = fontconfig_find_font_file("sans:weight=bold");
  spdlog::info("-->{}", fpp);


    FT_Library ft = Initialize();
    FT_Face face = LoadFace(ft, font_path);

    Render(face, text);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return 0;
}

// -------------------------------------------------------------------------------------
static int save_pgm(const char* path, const unsigned char* img, int w, int h);
static void put_pixel_u8(unsigned char* img, int w, int h, int x, int y, unsigned char v);
static void blend_glyph_bitmap(unsigned char* img, int w, int h,
                               const FT_Bitmap* bm, int x0, int y0);


static glm::ivec2 RenderGlyphByIndex(
    FT_Face face,
    FT_UInt glyphIndex,
    const glm::ivec2& pen,
    uint8_t* img, int W, int H)
{
    if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT)) {
        throw std::runtime_error(std::format("FT_Load_Glyph failed (glyphIndex={})", glyphIndex));
    }

    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
        throw std::runtime_error("FT_Render_Glyph failed");
    }

    FT_GlyphSlot g = face->glyph;

    const int x0 = pen.x + g->bitmap_left;
    const int y0 = pen.y - g->bitmap_top;
    spdlog::info("placed glypth at ({},{})", x0, y0);

    if (g->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
        // WICHTIG: blend_glyph_bitmap MUSS clippen (x0/y0 und bitmap bounds)
        blend_glyph_bitmap(img, W, H, &g->bitmap, x0, y0);
    } else {
        throw std::runtime_error(std::format("Unsupported pixel mode {}", (int)g->bitmap.pixel_mode));
    }

    return { (int)(g->advance.x >> 6), (int)(g->advance.y >> 6) };
}




int Render(FT_Face face, std::string_view utf8Text)
{
    const int W = 800, H = 200;
    std::vector<uint8_t> img((size_t)W * H, 0); // RAII, kein leak

    glm::ivec2 pen{50, 120};

    const auto cps = utf8Text;// Utf8ToCodepoints(utf8Text);

    FT_UInt prevGlyph = 0;
    const bool hasKerning = FT_HAS_KERNING(face);

    for (char32_t cp : cps) {
        // Hole Glyph explizit via Codepoint → GlyphIndex
        FT_UInt glyph = FT_Get_Char_Index(face, (FT_ULong)cp);

        if (glyph == 0) {
            // Fallback: '?' (wenn vorhanden)
            FT_UInt fallback = FT_Get_Char_Index(face, (FT_ULong)'?');
            if (fallback == 0) continue;
            glyph = fallback;
        }

        // Kerning (nur zwischen Glyphen, wenn Font das unterstützt)
        if (hasKerning && prevGlyph && glyph) {
            FT_Vector delta{};
            if (FT_Get_Kerning(face, prevGlyph, glyph, FT_KERNING_DEFAULT, &delta) == 0) {
                pen.x += (int)(delta.x >> 6);
                pen.y += (int)(delta.y >> 6);
            }
        }

        // Optional: early out wenn wir rechts raus sind
        if (pen.x >= W) break;

        pen += RenderGlyphByIndex(face, glyph, pen, img.data(), W, H);
        prevGlyph = glyph;
    }

    if (!save_pgm("out.pgm", img.data(), W, H)) return 2;
    return 0;
}





static void put_pixel_u8(unsigned char* img, int w, int h, int x, int y, unsigned char v)
{
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    img[y * w + x] = v;
}

// Blend a FreeType 8-bit coverage bitmap onto an 8-bit grayscale image (white text on black).
static void blend_glyph_bitmap(unsigned char* img, int w, int h,
                               const FT_Bitmap* bm, int x0, int y0)
{
    // FreeType bitmap buffer contains coverage values 0..255 (for FT_PIXEL_MODE_GRAY)
    for (int row = 0; row < (int)bm->rows; ++row) {
        for (int col = 0; col < (int)bm->width; ++col) {
            int x = x0 + col;
            int y = y0 + row;

            unsigned char a = bm->buffer[row * bm->pitch + col]; // coverage
            if (a == 0) continue;

            // simple "over" blend on grayscale: dst = dst + a*(255-dst)/255
            if (x < 0 || y < 0 || x >= w || y >= h) continue;
            unsigned char dst = img[y * w + x];
            unsigned int out = dst + (a * (255u - dst)) / 255u;
            img[y * w + x] = (unsigned char)out;
        }
    }
}

static int save_pgm(const char* path, const unsigned char* img, int w, int h)
{
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "P5\n%d %d\n255\n", w, h);
    fwrite(img, 1, (size_t)(w * h), f);
    fclose(f);
    return 1;
}





// Returns a malloc()'d UTF-8 path to the matched font file, or NULL on failure.
// Caller must free() the returned string.
char* fontconfig_find_font_file(const char* query)
{
    if (!query) return NULL;

    // Initialize fontconfig (safe to call multiple times; returns FcFalse on error).
    if (!FcInit()) {
        return NULL;
    }

    FcPattern* pat = FcNameParse((const FcChar8*)query);
    if (!pat) return NULL;

    // Apply config substitutions (aliases, defaults, etc.)
    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result = FcResultNoMatch;
    FcPattern* match = FcFontMatch(NULL, pat, &result);

    FcChar8* file = NULL;
    char* out = NULL;

    if (match && FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file) {
        // Copy to a normal malloc() buffer so caller can free() easily.
        size_t n = strlen((const char*)file);
        out = (char*)malloc(n + 1);
        if (out) {
            memcpy(out, (const char*)file, n + 1);
        }
    }

    if (match) FcPatternDestroy(match);
    FcPatternDestroy(pat);

    return out;
}







#if 0
#include <hb.h>
#include <hb-ft.h>

int RenderHB(FT_Face face, std::string_view utf8Text)
{
    const int W = 800, H = 200;
    std::vector<uint8_t> img((size_t)W * H, 0);
    glm::ivec2 pen{50, 120};

    hb_font_t* hb_font = hb_ft_font_create_referenced(face);
    hb_buffer_t* buf = hb_buffer_create();

    hb_buffer_add_utf8(buf, utf8Text.data(), (int)utf8Text.size(), 0, (int)utf8Text.size());

    // Auto: erkennt Script/Direction oft selbst; für safety könntest du setzen:
    hb_buffer_guess_segment_properties(buf);

    // Shaping: macht aus Text -> Glyph IDs + Positioning
    hb_shape(hb_font, buf, nullptr, 0);

    unsigned int count = 0;
    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, &count);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &count);

    // HarfBuzz arbeitet in 26.6 fixed point (wie FreeType advances)
    for (unsigned i = 0; i < count; ++i) {
        const FT_UInt glyphIndex = infos[i].codepoint; // das ist jetzt die Glyph-ID!
        const int x_off = pos[i].x_offset >> 6;
        const int y_off = pos[i].y_offset >> 6;

        glm::ivec2 drawPen = pen + glm::ivec2{x_off, -y_off}; // y-Achse beachten (Baseline)
        pen += RenderGlyphByIndex(face, glyphIndex, drawPen, img.data(), W, H);

        // advance NICHT aus FreeType nehmen, sondern aus HarfBuzz!
        pen.x += (pos[i].x_advance >> 6);
        pen.y -= (pos[i].y_advance >> 6);

        if (pen.x >= W) break;
    }

    hb_buffer_destroy(buf);
    hb_font_destroy(hb_font);

    if (!save_pgm("out.pgm", img.data(), W, H)) return 2;
    return 0;
}

#endif