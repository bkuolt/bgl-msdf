
#include <hb.h>
#include <hb-ft.h>

#include <string>

// void shape()
// positions()

// void Render("text")
// void Render(hb_glyph_info_t,     hb_glyph_position_t* pos);

int RenderHB(FT_Face face, std::string_view utf8Text)
{

    glm::ivec2 pen{50, 120};

    hb_font_t *hb_font = hb_ft_font_create_referenced(face);
    hb_buffer_t *buf = hb_buffer_create();

    hb_buffer_add_utf8(buf, utf8Text.data(), (int)utf8Text.size(), 0, (int)utf8Text.size());

    // Auto: erkennt Script/Direction oft selbst; für safety könntest du setzen:
    hb_buffer_guess_segment_properties(buf);

    // Shaping: macht aus Text -> Glyph IDs + Positioning
    hb_shape(hb_font, buf, nullptr, 0);

    unsigned int count = 0;
    hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buf, &count);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, &count);

    spdlog::info("glyphs: {}", count);

    // HarfBuzz arbeitet in 26.6 fixed point (wie FreeType advances)
    for (unsigned i = 0; i < count; ++i)
    {

        const FT_UInt glyphIndex = infos[i].codepoint; // das ist jetzt die Glyph-ID!
        const int x_off = pos[i].x_offset >> 6;
        const int y_off = pos[i].y_offset >> 6;

        glm::ivec2 drawPen = pen + glm::ivec2{x_off, -y_off}; // y-Achse beachten (Baseline)
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
