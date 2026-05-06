/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --font Phosphor.ttf --size 18 --bpp 1 --format lvgl --no-compress -r 0xE210,0xE5C6 -o src/lv_font_phosphor_18.c
 ******************************************************************************/

#include <lvgl.h>

#ifndef LV_FONT_PHOSPHOR_18
#define LV_FONT_PHOSPHOR_18 1
#endif

#if LV_FONT_PHOSPHOR_18

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+E210 "" */
    0x0, 0x0, 0xf0, 0x19, 0x81, 0x8, 0x20, 0x44,
    0x2, 0x40, 0x28, 0x1, 0x80, 0x18, 0x1, 0x80,
    0x58, 0xd, 0x41, 0xa4, 0x32, 0x30, 0xc0, 0xf0,

    /* U+E5C6 "" */
    0x1c, 0x1, 0x10, 0x18, 0x80, 0xc6, 0x66, 0x37,
    0xb5, 0xa5, 0xad, 0xed, 0x60, 0x6b, 0x2, 0x48,
    0x27, 0x21, 0x45, 0xa, 0x28, 0x6e, 0xc1, 0x8c,
    0x7, 0xc0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 288, .box_w = 12, .box_h = 16, .ofs_x = 3, .ofs_y = 1},
    {.bitmap_index = 24, .adv_w = 288, .box_w = 13, .box_h = 16, .ofs_x = 4, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x3b6
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 57872, .range_length = 951, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_phosphor_18 = {
#else
lv_font_t lv_font_phosphor_18 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_PHOSPHOR_18*/

