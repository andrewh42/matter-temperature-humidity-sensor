/*
 * SPDX-License-Identifier: OFL-1.1
 */

/*******************************************************************************
 * Size: 20 px
 * Bpp: 1
 * Opts: --font SplineSans/static/SplineSans-Medium.ttf --size 20 --bpp 1 --format lvgl --no-compress -r 0x20,0x25,0x2E,0x30-0x3A,0x43-0x45,0x48,0x4E-0x4F,0x52,0xB0 -o lv_font_splinesans_medium_20.c
 ******************************************************************************/

#include <lvgl.h>

#ifndef LV_FONT_SPLINESANS_MEDIUM_20
#define LV_FONT_SPLINESANS_MEDIUM_20 1
#endif

#if LV_FONT_SPLINESANS_MEDIUM_20

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0025 "%" */
    0x3c, 0x10, 0x7e, 0x30, 0xc6, 0x20, 0xc6, 0x60,
    0xc6, 0xc0, 0xfc, 0xc0, 0x79, 0x9c, 0x1, 0xbe,
    0x3, 0x63, 0x3, 0x63, 0x6, 0x63, 0x4, 0x63,
    0xc, 0x3e, 0x18, 0x1c,

    /* U+002E "." */
    0xff, 0x80,

    /* U+0030 "0" */
    0x1e, 0x1f, 0xe6, 0x1b, 0x87, 0xc0, 0xf0, 0x3c,
    0xf, 0x3, 0xc0, 0xf0, 0x3e, 0x1d, 0x86, 0x7f,
    0x87, 0x80,

    /* U+0031 "1" */
    0x1f, 0xf6, 0x31, 0x8c, 0x63, 0x18, 0xc6, 0x31,
    0x8c,

    /* U+0032 "2" */
    0x3e, 0x3f, 0xb8, 0xfc, 0x3c, 0x18, 0xc, 0xc,
    0xe, 0xe, 0xe, 0x6, 0x7, 0x7, 0xff, 0xfc,

    /* U+0033 "3" */
    0x1e, 0x1f, 0xe6, 0x19, 0x86, 0x3, 0x83, 0xc0,
    0xf0, 0x6, 0x0, 0xf0, 0x3c, 0xf, 0x87, 0x7f,
    0x8f, 0xc0,

    /* U+0034 "4" */
    0x3, 0x80, 0xf0, 0x1e, 0x6, 0xc1, 0xd8, 0x33,
    0xe, 0x61, 0x8c, 0x71, 0x8f, 0xff, 0xff, 0x80,
    0xc0, 0x18, 0x3, 0x0,

    /* U+0035 "5" */
    0x7f, 0x3f, 0x98, 0x1c, 0xd, 0xe7, 0xf9, 0x8e,
    0x3, 0x1, 0xe0, 0xf0, 0x7c, 0x67, 0xf1, 0xe0,

    /* U+0036 "6" */
    0x1e, 0x7, 0x3, 0x81, 0xc0, 0x6f, 0x37, 0xee,
    0x1f, 0x3, 0xc0, 0xf0, 0x3e, 0xd, 0x86, 0x7f,
    0x87, 0x80,

    /* U+0037 "7" */
    0xff, 0xbf, 0xe0, 0x18, 0xc, 0x3, 0x1, 0xc0,
    0x60, 0x38, 0xc, 0x3, 0x1, 0xc0, 0x60, 0x38,
    0xe, 0x0,

    /* U+0038 "8" */
    0x1f, 0xf, 0xe7, 0x1d, 0x83, 0x71, 0xcf, 0xe3,
    0xf9, 0x87, 0xc0, 0xf0, 0x3c, 0xf, 0x87, 0x7f,
    0x8f, 0xc0,

    /* U+0039 "9" */
    0x1e, 0x1f, 0xee, 0x1b, 0x3, 0xc0, 0xf0, 0x3e,
    0x1d, 0xff, 0x3f, 0xc0, 0x60, 0x38, 0x1c, 0xe,
    0x7, 0x80,

    /* U+003A ":" */
    0xff, 0x80, 0x0, 0xff, 0x80,

    /* U+0043 "C" */
    0xf, 0x83, 0xfe, 0x70, 0x76, 0x3, 0xe0, 0x3c,
    0x0, 0xc0, 0xc, 0x0, 0xc0, 0xc, 0x0, 0x60,
    0x37, 0x7, 0x3f, 0xe0, 0xf8,

    /* U+0044 "D" */
    0xfe, 0x1f, 0xf3, 0x7, 0x60, 0x6c, 0x7, 0x80,
    0xf0, 0x1e, 0x3, 0xc0, 0x78, 0xf, 0x3, 0x60,
    0xef, 0xf9, 0xfc, 0x0,

    /* U+0045 "E" */
    0xff, 0xff, 0xf0, 0x18, 0xc, 0x6, 0x3, 0xff,
    0xff, 0xc0, 0x60, 0x30, 0x18, 0xf, 0xff, 0xfc,

    /* U+0048 "H" */
    0xc0, 0xf0, 0x3c, 0xf, 0x3, 0xc0, 0xf0, 0x3f,
    0xff, 0xff, 0xc0, 0xf0, 0x3c, 0xf, 0x3, 0xc0,
    0xf0, 0x30,

    /* U+004E "N" */
    0xe0, 0x7c, 0xf, 0xc1, 0xfc, 0x3d, 0x87, 0xb8,
    0xf3, 0x9e, 0x33, 0xc7, 0x78, 0x6f, 0xf, 0xe0,
    0xfc, 0xf, 0x81, 0xc0,

    /* U+004F "O" */
    0xf, 0x81, 0xff, 0x1c, 0x1c, 0xc0, 0x6c, 0x1,
    0xe0, 0xf, 0x0, 0x78, 0x3, 0xc0, 0x1e, 0x1,
    0xd8, 0xc, 0xe0, 0xe3, 0xfe, 0x7, 0xc0,

    /* U+0052 "R" */
    0xfe, 0x1f, 0xe3, 0xe, 0x60, 0xcc, 0x19, 0x87,
    0x3f, 0xc7, 0xe0, 0xc6, 0x18, 0xe3, 0xe, 0x61,
    0xcc, 0x1d, 0x81, 0x80,

    /* U+00B0 "°" */
    0x7b, 0xfc, 0xf3, 0xfd, 0xe0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 57, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 290, .box_w = 16, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 29, .adv_w = 69, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 31, .adv_w = 190, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 132, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 175, .box_w = 9, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 74, .adv_w = 180, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 178, .box_w = 11, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 180, .box_w = 9, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 180, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 168, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 195, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 180, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 87, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 205, .adv_w = 219, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 217, .box_w = 11, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 181, .box_w = 9, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 219, .box_w = 10, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 226, .box_w = 11, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 300, .adv_w = 235, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 199, .box_w = 11, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 343, .adv_w = 140, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 9}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x5, 0xe, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x23, 0x24,
    0x25, 0x28, 0x2e, 0x2f, 0x32, 0x90
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 145, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 22, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11, 12, 13,
    14, 15, 16, 16, 17, 18, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 15, 15, 17, 15, 18
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, -4, -24, 0, 6, 2, 2,
    -1, -16, 0, -20, 0, 0, 0, 0,
    0, 0, -2, -4, 0, 0, 0, 1,
    3, 6, 5, -5, 1, 7, -2, 0,
    0, 0, 0, 0, 0, -6, 0, -5,
    -3, -1, -2, -2, -3, -6, -4, 0,
    -10, 0, 0, 0, 0, 0, 0, 7,
    2, 0, 2, 0, 2, 0, 0, 0,
    0, 4, -2, 0, 0, 0, 0, 0,
    0, 4, 7, -1, 0, 7, 10, 3,
    1, -3, 1, 1, -3, 0, 0, 0,
    0, 0, 0, 0, 4, -8, 0, 12,
    15, 9, 6, -7, 5, 0, -4, 0,
    0, 0, 0, 0, 0, 2, 4, -4,
    0, 5, 9, 2, 4, 0, 1, 0,
    -4, 0, 0, 0, 0, 0, 0, 4,
    7, -16, -3, 7, 11, 4, 6, -14,
    2, 0, -3, 0, 0, 0, 0, -4,
    0, -34, -1, 0, 0, -1, -16, 0,
    -9, 7, -5, 0, -14, 0, 0, 0,
    0, 0, 0, 0, 1, -5, 0, 1,
    5, 0, 0, -5, 0, 0, -5, 0,
    0, 0, 0, 0, 0, -14, 6, 0,
    0, 0, 0, 2, 0, -4, 0, 6,
    -3, 0, 0, 0, 0, 0, 0, 0,
    -2, -12, -3, -3, -2, -2, -2, -16,
    -5, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -9, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 4,
    0, 0, 5, 0, 0, 11, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -12, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    -1, -1, -4, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 18,
    .right_class_cnt     = 18,
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
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 1,
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
const lv_font_t lv_font_splinesans_medium_20 = {
#else
lv_font_t lv_font_splinesans_medium_20 = {
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



#endif /*#if LV_FONT_SPLINESANS_MEDIUM_20*/

