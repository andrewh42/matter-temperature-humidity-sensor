/*
 * SPDX-License-Identifier: OFL-1.1
 */

/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --font SplineSans/static/SplineSans-Medium.ttf --size 18 --bpp 1 --format lvgl --no-compress -r 0x20,0x25,0x2B,0x2D-0x2E,0x30-0x3A,0x43-0x45,0x48,0x4E-0x4F,0x52-0x54,0x78,0xB0 -o lv_font_splinesans_medium_18.c
 ******************************************************************************/

#include <lvgl.h>

#ifndef LV_FONT_SPLINESANS_MEDIUM_18
#define LV_FONT_SPLINESANS_MEDIUM_18 1
#endif

#if LV_FONT_SPLINESANS_MEDIUM_18

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0025 "%" */
    0x78, 0x23, 0xf1, 0xc, 0xc4, 0x33, 0x20, 0xcd,
    0x83, 0xf4, 0x7, 0xb7, 0x80, 0xbf, 0x4, 0xcc,
    0x13, 0x30, 0x8c, 0xc2, 0x33, 0x10, 0x78,

    /* U+002B "+" */
    0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18,

    /* U+002D "-" */
    0xff, 0xc0,

    /* U+002E "." */
    0xff, 0x80,

    /* U+0030 "0" */
    0x3e, 0x3f, 0x98, 0xd8, 0x3c, 0x1e, 0xf, 0x7,
    0x83, 0xc1, 0xe0, 0xd8, 0xcf, 0xe3, 0xe0,

    /* U+0031 "1" */
    0x1f, 0xfe, 0x31, 0x8c, 0x63, 0x18, 0xc6, 0x31,
    0x80,

    /* U+0032 "2" */
    0x1e, 0x3f, 0x9c, 0xec, 0x36, 0x18, 0x1c, 0xc,
    0xc, 0xc, 0xe, 0xe, 0x7, 0xf7, 0xf8,

    /* U+0033 "3" */
    0x1e, 0x1f, 0xd8, 0x6c, 0x30, 0x38, 0x78, 0x3c,
    0x7, 0x1, 0xe0, 0xf8, 0xef, 0xe3, 0xe0,

    /* U+0034 "4" */
    0x7, 0x3, 0xc0, 0xf0, 0x6c, 0x1b, 0xc, 0xc3,
    0x31, 0xcc, 0x63, 0x3f, 0xff, 0xfc, 0xc, 0x3,
    0x0,

    /* U+0035 "5" */
    0x7f, 0x3f, 0x98, 0xc, 0x7, 0xe3, 0xf9, 0x8e,
    0x3, 0x1, 0xf0, 0xd8, 0xef, 0xe3, 0xe0,

    /* U+0036 "6" */
    0x1c, 0x1c, 0xc, 0xc, 0x7, 0xe6, 0xfb, 0x8f,
    0x83, 0xc1, 0xe0, 0xd8, 0xef, 0xe3, 0xe0,

    /* U+0037 "7" */
    0xff, 0x7f, 0x80, 0xc0, 0xc0, 0x60, 0x30, 0x30,
    0x18, 0x1c, 0xc, 0x6, 0x7, 0x3, 0x0,

    /* U+0038 "8" */
    0x1e, 0x3f, 0xd8, 0x6c, 0x36, 0x39, 0xf9, 0xfd,
    0xc7, 0xc1, 0xe0, 0xf8, 0xef, 0xe3, 0xe0,

    /* U+0039 "9" */
    0x3e, 0x3f, 0xb8, 0xd8, 0x3c, 0x1e, 0xf, 0x8e,
    0xff, 0x3f, 0x1, 0x81, 0x81, 0xc1, 0xc0,

    /* U+003A ":" */
    0xff, 0x80, 0x7, 0xfc,

    /* U+0043 "C" */
    0x1f, 0x87, 0xf9, 0xc3, 0xb0, 0x3c, 0x1, 0x80,
    0x30, 0x6, 0x0, 0xc0, 0xc, 0xd, 0xc3, 0x9f,
    0xe1, 0xf8,

    /* U+0044 "D" */
    0xfe, 0x3f, 0xcc, 0x3b, 0x7, 0xc0, 0xf0, 0x3c,
    0xf, 0x3, 0xc0, 0xf0, 0x6c, 0x3b, 0xfc, 0xfe,
    0x0,

    /* U+0045 "E" */
    0xff, 0xff, 0xc0, 0xc0, 0xc0, 0xff, 0xff, 0xc0,
    0xc0, 0xc0, 0xc0, 0xff, 0xff,

    /* U+0048 "H" */
    0xc1, 0xe0, 0xf0, 0x78, 0x3c, 0x1f, 0xff, 0xff,
    0x83, 0xc1, 0xe0, 0xf0, 0x78, 0x3c, 0x18,

    /* U+004E "N" */
    0xe0, 0xf8, 0x3f, 0xf, 0xc3, 0xd8, 0xf7, 0x3c,
    0xcf, 0x1b, 0xc6, 0xf0, 0xfc, 0x3f, 0x7, 0xc1,
    0xc0,

    /* U+004F "O" */
    0x1f, 0x7, 0xf1, 0xc7, 0x70, 0x7c, 0x7, 0x80,
    0xf0, 0x1e, 0x3, 0xc0, 0x7c, 0x1d, 0xc7, 0x1f,
    0xc1, 0xf0,

    /* U+0052 "R" */
    0xfc, 0x7f, 0x31, 0xd8, 0x6c, 0x36, 0x3b, 0xf9,
    0xf8, 0xce, 0x63, 0x31, 0xd8, 0x7c, 0x18,

    /* U+0053 "S" */
    0x1e, 0x3f, 0x98, 0xcc, 0x36, 0x1, 0xc0, 0x78,
    0xe, 0x1, 0xe0, 0xf8, 0x6f, 0xe3, 0xe0,

    /* U+0054 "T" */
    0xff, 0xff, 0xc6, 0x3, 0x1, 0x80, 0xc0, 0x60,
    0x30, 0x18, 0xc, 0x6, 0x3, 0x1, 0x80,

    /* U+0078 "x" */
    0x61, 0x9c, 0xe3, 0x30, 0x78, 0x1e, 0x7, 0x81,
    0xe0, 0xcc, 0x73, 0x98, 0x60,

    /* U+00B0 "°" */
    0x76, 0xf7, 0xb7, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 51, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 261, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 24, .adv_w = 161, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 32, .adv_w = 116, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 34, .adv_w = 62, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 171, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 51, .adv_w = 119, .box_w = 5, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 60, .adv_w = 157, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 162, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 161, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 162, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 162, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 151, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 152, .adv_w = 175, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 162, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 79, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 186, .adv_w = 197, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 195, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 163, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 197, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 249, .adv_w = 204, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 266, .adv_w = 211, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 179, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 299, .adv_w = 166, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 161, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 329, .adv_w = 150, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 126, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 8}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x5, 0xb, 0xd, 0xe, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
    0x23, 0x24, 0x25, 0x28, 0x2e, 0x2f, 0x32, 0x33,
    0x34, 0x58, 0x90
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 145, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 27, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
    14, 15, 16, 17, 18, 18, 19, 20,
    21, 22, 23, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 17, 20, 19,
    21, 22, 23, 24
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, 0, -3, -2, -1,
    -1, -1, -1, -6, -1, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, 0, 0, 0, -5, 0,
    0, 0, 0, 0, -3, -21, 0, 6,
    2, 2, -1, -15, 0, -18, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, -1, 0, -3, 0, 0, 0, 1,
    2, 5, 5, -4, 1, 6, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -5, 0, -5, -3, -1,
    -2, -1, -3, -6, -4, 0, -9, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 6, 2, 0, 2, 0,
    2, 0, 0, 0, 0, 4, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 6, -1, 0, 6,
    9, 3, 1, -2, 1, 1, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 3, -7, 0, 10,
    13, 8, 5, -6, 5, 0, -4, 0,
    0, 0, 0, 0, 0, -1, 0, 0,
    0, 0, 0, 1, 4, -3, 0, 5,
    8, 2, 4, 0, 1, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 3, 6, -14, -3, 7,
    10, 3, 5, -12, 2, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, -3,
    0, 0, 0, -31, -1, 0, 0, -1,
    -14, 0, -8, 6, -5, 0, -13, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, -4, 0, 1,
    4, 0, 0, -5, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -12, 5, 0, 0, 0,
    0, 2, 0, -3, 0, 5, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, -10, -3, -3,
    -2, -1, -2, -14, -5, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, -4, 0, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 4,
    0, 0, 0, 5, 2, -7, 0, 0,
    0, 0, 0, 10, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, 0, 0, -2, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, 0, 0,
    0, 0, 0, -11, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -10, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    -1, -1, -1, -3, -1, -11, 0, 0,
    0, 0, 0, 7, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 0, 3, -3, 0, 0,
    0, 0, 0, -30, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -10,
    -3, -3, -3, -9, -4, 3, -18, 0,
    0, 0, -3, 3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 3, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 23,
    .right_class_cnt     = 24,
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
const lv_font_t lv_font_splinesans_medium_18 = {
#else
lv_font_t lv_font_splinesans_medium_18 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_SPLINESANS_MEDIUM_18*/

