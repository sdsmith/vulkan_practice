#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include "status.h"
#include <string>

class Free_Type_Wrapper {
public:
    Status initialize();
    Status load_font(const std::string& path);

    /**
     * \brief Set the font size in terms of physical units. Calculates the, 
     * possibly factional, pixel size for each glyph.
     * 
     * Characters are measured in points, a physical distance. A point is 1/72 
     * of an inch.
     * Device resolution is measured in DPI, dots-per-inch.
     *
     * \param char_width_pt Character width in points. 0 if same as height.
     * \param char_height_pt Character height in points. 0 if same as width.
     * \param device_width_dpi Device width in DPI. 0 is same as height.
     * \param device_height_dpi Device height in DPI. 0 is same as width.
     */
    Status set_font_size_physical(float char_width_pt, float char_height_pt, 
        uint32_t device_width_dpi, uint32_t device_height_dpi);

    /**
     * \brief Set the for size in pixels.
     *
     * \param pixel_width Character width in pixels. 0 if same as height.
     * \param pixel_height Character height in pixels. 0 if same as width.
     */
    Status set_font_size_pixel(uint32_t pixel_width, uint32_t pixel_height);

    Status load_glyph_bitmap(FT_ULong charcode);

private:
    bool m_initialized = false;
    FT_Library m_library; //! library handle
    FT_Face m_face; //! font face handle
};