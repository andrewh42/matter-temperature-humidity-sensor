/*
 * SPDX-License-Identifier: OFL-1.1
 */

/*******************************************************************************
 * Size: 20 px
 * Bpp: 1
 * Opts: --font Rubik/static/Rubik-Medium.ttf --size 20 --bpp 1 --format lvgl --no-compress -r 0x20,0x25,0x43,0x48,0x52,0xB0 -o lv_font_rubik_medium_20.c
 ******************************************************************************/

#include <lvgl.h>

#ifndef LV_FONT_RUBIK_MEDIUM_20
#define LV_FONT_RUBIK_MEDIUM_20 1
#endif

#if LV_FONT_RUBIK_MEDIUM_20

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0025 "%" */
    0x78, 0x1b, 0xf0, 0xcc, 0xc7, 0x33, 0x18, 0xcc,
    0xc3, 0x37, 0xf, 0xd8, 0x1e, 0xde, 0x6, 0xfc,
    0x3b, 0x30, 0xcc, 0xc6, 0x33, 0x38, 0xcc, 0xc3,
    0xf6, 0x7, 0x80,

    /* U+0043 "C" */
    0x1f, 0x83, 0xfe, 0x70, 0xee, 0x7, 0xe0, 0x7e,
    0x0, 0xe0, 0xe, 0x0, 0xe0, 0xe, 0x0, 0xe0,
    0x7e, 0x7, 0x70, 0xe3, 0xfe, 0x1f, 0x80,

    /* U+0048 "H" */
    0xe0, 0x7e, 0x7, 0xe0, 0x7e, 0x7, 0xe0, 0x7e,
    0x7, 0xe0, 0x7f, 0xff, 0xff, 0xfe, 0x7, 0xe0,
    0x7e, 0x7, 0xe0, 0x7e, 0x7, 0xe0, 0x70,

    /* U+0052 "R" */
    0xff, 0x1f, 0xfb, 0x87, 0xf0, 0x7e, 0xf, 0xc1,
    0xf8, 0x7f, 0xfe, 0xff, 0x1c, 0x73, 0x8e, 0x70,
    0xee, 0x1d, 0xc1, 0xf8, 0x38,

    /* U+00B0 "°" */
    0x38, 0xfb, 0x1e, 0x3c, 0x6f, 0x8e, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 73, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 251, .box_w = 14, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 28, .adv_w = 219, .box_w = 12, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 51, .adv_w = 233, .box_w = 12, .box_h = 15, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 74, .adv_w = 215, .box_w = 11, .box_h = 15, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 8}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x5, 0x23, 0x28, 0x32, 0x90
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 145, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 6, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
const lv_font_t lv_font_rubik_medium_20 = {
#else
lv_font_t lv_font_rubik_medium_20 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_RUBIK_MEDIUM_20*/

