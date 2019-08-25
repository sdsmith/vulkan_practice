#include "freetype_wrapper.h"

#include <cassert>

#define FT_ERROR(Expression)                             \
    do {                                                 \
        FT_Error error_ = (Expression);                  \
        if (error_) {                                    \
            log_error("%s:%d: FreeType error\n", __FILE__, __LINE__); \
            return !STATUS_OK;                           \
        }                                                \
    } while (0)

/*
	- font file loads into a \a face (FT_New_Face)
	- a glyph is taken from a face (FT_Get_Char_Index)
 */

Status Free_Type_Wrapper::initialize() {
    FT_ERROR(FT_Init_FreeType(&m_library));

    m_initialized = true;
    return STATUS_OK;
}

Status Free_Type_Wrapper::load_font(const std::string& path) {
    assert(m_initialized);

    FT_Error err = FT_New_Face(m_library, path.c_str(), 0, &m_face);
    if (err == FT_Err_Unknown_File_Format) {
        log_error("Unknown font format: %s\n", path.c_str()); // TODO: find the extension
    }
    FT_ERROR(err);

    return STATUS_OK;
}

Status Free_Type_Wrapper::set_font_size_physical(float char_width_pt, float char_height_pt, uint32_t device_width_dpi, uint32_t device_height_dpi) {
    FT_ERROR(FT_Set_Char_Size(m_face, char_width_pt * 64.f, char_height_pt * 64.f, device_width_dpi, device_height_dpi));
    return STATUS_OK;
}

Status Free_Type_Wrapper::set_font_size_pixel(uint32_t pixel_width, uint32_t pixel_height) {
    FT_ERROR(FT_Set_Pixel_Sizes(m_face, pixel_width, pixel_height));
}

//! \param charcode UTF-32 character code.
Status Free_Type_Wrapper::load_glyph_bitmap(FT_ULong charcode) {
    // Get glyph index corresponding to UTF-32 character code from charcode map.
    FT_UInt glyph_index = FT_Get_Char_Index(m_face, charcode);

    // Load glyph into single face glyph slot (face->glyph)
    FT_Int32 load_flags = FT_LOAD_DEFAULT;
    FT_ERROR(FT_Load_Glyph(m_face, glyph_index, load_flags));

    // If it's not a bitmap, convert it
    if (m_face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL;
        FT_ERROR(FT_Render_Glyph(m_face->glyph, render_mode));
    }

    // NOTE: Bitmap can be accessed through glyph->bitmap and positioned with 
    // glyph->bitmap_left and glyph->bitmap_top. bitmap_left is the horizontal 
    // distance form the current pen position to the leftmost border of the 
    // glyph bitmap. bitmap_top is the vertical distnace from the pen position 
    // (on the baseline) to the topmost border of the glyph bitmap. Positive to
    // indicate an upward distance.
    //
    // NOTE: For optimal rendering on a screen the bitmap should be used as an 
    // alpha channel in linear blending with gamma correction.
}

// TODO: Convert bitmap to texture. Keep cached textures for run.
class Glyph {
public:
    Glyph(FT_Face face, FT_UInt glyph_index);
};

void Renderer::draw(float x, float y, const std::string& text)
{
	int cur_x_pos = x;
	for (char c : text) {
		const Glyph& glyph = GetGlyph(c);
		const Texture* tex = glyph->GetTexture();

		if (tex) {
			SetTexture(tex);
			float x0 = cur_x_pos + glyph->GetLeft();
			float y0 = y - glyph->GetTop();
			DrawSprite(x0, y0, tex->GetWidth(), tex->GetHeight());
		} else {
			// TODO: Draw some default character (the box with an X through it?)
		}
	
		cur_x_pos += glyph->GetHorizontalAdvance();
	}
}