/*
 *  mpui_struct.h: libmpui structures storage.
 *  Originally developped for the GeeXboX project.
 *  Copyright (C) 2004-2005  Aurelien Jacobs
 *  Copyright (C) 2004-2005  Benjamin Zores
 *
 *   This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *   This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MPUI_STRUCT_H
#define MPUI_STRUCT_H

#include <stdlib.h>
#include <dirent.h>

#include "config.h"
#include "libvo/font_load.h"
#include "input/input.h"


typedef void (* mpui_list_free_func_t) (void *ptr);

typedef enum mpui_when_focused mpui_when_focused_t;
typedef enum mpui_type mpui_type_t;
typedef struct mpui_element mpui_element_t;
typedef struct mpui_container mpui_container_t;
typedef struct mpui_focus_box mpui_focus_box_t;
typedef int mpui_size_t;
typedef struct mpui_coord mpui_coord_t;
typedef struct mpui_color mpui_color_t;
typedef struct mpui_keymaps mpui_keymaps_t;
typedef struct mpui_keymapping mpui_keymapping_t;

typedef enum mpui_encoding mpui_encoding_t;
typedef struct mpui_strings mpui_strings_t;
typedef struct mpui_string mpui_string_t;
typedef struct mpui_str mpui_str_t;
typedef struct mpui_images mpui_images_t;
typedef struct mpui_raw_image mpui_raw_image_t;
typedef struct mpui_image mpui_image_t;
typedef struct mpui_img mpui_img_t;
typedef struct mpui_fonts mpui_fonts_t;
typedef struct mpui_font mpui_font_t;
typedef enum mpui_action_when mpui_action_when_t;
typedef struct mpui_action mpui_action_t;
typedef struct mpui_filetype mpui_filetype_t;
typedef struct mpui_filetypes mpui_filetypes_t;
typedef enum mpui_match mpui_match_t;
typedef enum mpui_flags mpui_flags_t;
typedef struct mpui_objects mpui_objects_t;
typedef struct mpui_object mpui_object_t;
typedef struct mpui_obj mpui_obj_t;
typedef enum mpui_orientation mpui_orientation_t;
typedef enum mpui_alignment mpui_alignment_t;
typedef struct mpui_menuitem mpui_menuitem_t;
typedef struct mpui_allmenuitem mpui_allmenuitem_t;
typedef struct mpui_menus mpui_menus_t;
typedef struct mpui_menu mpui_menu_t;
typedef struct mpui_mnu mpui_mnu_t;
typedef struct mpui_browser mpui_browser_t;
typedef struct mpui_playlist mpui_playlist_t;
typedef struct mpui_infos mpui_infos_t;
typedef struct mpui_info mpui_info_t;
typedef struct mpui_inf mpui_inf_t;
typedef struct mpui_tag mpui_tag_t;
typedef struct mpui_pic mpui_pic_t;
typedef struct mpui_sys mpui_sys_t;
typedef enum mpui_slideshow_mode mpui_slideshow_mode_t;
typedef struct mpui_slideshow mpui_slideshow_t;
typedef struct mpui_popup mpui_popup_t;
typedef struct mpui_popups mpui_popups_t;
typedef struct mpui_screens mpui_screens_t;
typedef struct mpui_screen mpui_screen_t;
typedef struct mpui mpui_t;

enum mpui_flags {
  MPUI_FLAG_ABSOLUTE  = 0x01,
  MPUI_FLAG_DYNAMIC   = 0x02,
  MPUI_FLAG_NOCOORD   = 0x04,
  MPUI_FLAG_CONTAINER = 0x08,
  MPUI_FLAG_FOCUS_BOX = 0x10,
  MPUI_FLAG_FOCUSABLE = 0x20,
  MPUI_FLAG_OWNER     = 0x40,
  MPUI_FLAG_HIDDEN    = 0x80,
};

enum mpui_when_focused {
  MPUI_DISPLAY_NORMAL,
  MPUI_DISPLAY_REALLY_NORMAL,
  MPUI_DISPLAY_FOCUSED,
  MPUI_DISPLAY_REALLY_FOCUSED,
  MPUI_DISPLAY_ALWAYS,
};

enum mpui_orientation {
  MPUI_ORIENTATION_H = 0x01,
  MPUI_ORIENTATION_V = 0x02,
};

enum mpui_alignment {
  MPUI_ALIGNMENT_LEFT,
  MPUI_ALIGNMENT_TOP,
  MPUI_ALIGNMENT_CENTER,
  MPUI_ALIGNMENT_RIGHT,
  MPUI_ALIGNMENT_BOTTOM,
};

enum mpui_encoding {
  MPUI_ENCODING_ISO_8859_1,
  MPUI_ENCODING_UTF8,
  MPUI_ENCODING_UTF16,
};

enum mpui_action_when {
  MPUI_WHEN_VALIDATE = 0x01,
  MPUI_WHEN_FOCUS    = 0x02,
  MPUI_WHEN_UNFOCUS  = 0x04,
};

enum mpui_match {
  MPUI_MATCH_ALL,
  MPUI_MATCH_DIR,
  MPUI_MATCH_EXT,
};

enum mpui_slideshow_mode {
  MPUI_SLIDESHOW_MODE_1_1,
  MPUI_SLIDESHOW_MODE_ZOOM,
};

enum mpui_type {
  MPUI_ANY,
  MPUI_STR,
  MPUI_IMG,
  MPUI_OBJ,
  MPUI_MNU,
  MPUI_MENUITEM,
  MPUI_ALLMENUITEM,
  MPUI_POPUP,
  MPUI_INF,
  MPUI_SLIDESHOW,
};

struct mpui_coord {
  mpui_size_t val;
  char *str;
};

struct mpui_element {
  char *id;
  mpui_type_t type;
  mpui_flags_t flags;
  mpui_coord_t x, y;
  mpui_coord_t w, h;
  mpui_when_focused_t when_focused;
};

struct mpui_container {
  mpui_element_t element;
  mpui_element_t **elements;
  mpui_action_t **actions;
  mpui_keymaps_t *keymaps;
};

struct mpui_focus_box {
  mpui_container_t container;
  mpui_element_t **focus;
  mpui_orientation_t orientation, scrolling;
  mpui_size_t xoffset, yoffset;
};

struct mpui_color {
  unsigned char r, g, b;
  unsigned char y, u, v;
};

struct mpui_keymaps {
  char *id;
  int num;
  mp_cmd_bind_t *binds;
};

struct mpui_keymapping {
  mpui_keymaps_t **keymaps;
};

struct mpui_strings {
  char *fonts;
  char *lang;
  mpui_string_t **strings;
};

struct mpui_string {
  char *id;
  unsigned char *text;
  mpui_encoding_t encoding;
};

struct mpui_str {
  mpui_element_t element;
  mpui_string_t *string;
  mpui_font_t *font;
  int size;
  mpui_color_t *color;
  mpui_color_t *focused_color;
  mpui_color_t *really_focused_color;
};

struct mpui_images {
  mpui_image_t **images;
};

struct mpui_raw_image {
  int format;
  int alpha;
  int bpp;
  int w, h;
  int stride;
  unsigned char *data;
};

struct mpui_image {
  char *id;
  char *file;
  mpui_raw_image_t raw;
  mpui_size_t x, y, w, h;
  int format;         /* one of the IMGFMT_* */
  int alpha;          /* does the image contain an alpha channel */
  int bpp;            /* byte per pixel (only for packed format) */
  int num_planes;     /* 1 for packed format, generaly 3 for planar */
  unsigned char *planes[5];
  unsigned int stride[5];
  int chroma_width, chroma_height;  /* only for planar formats */
  int dup;
};

struct mpui_img {
  mpui_element_t element;
  mpui_image_t *image;
  mpui_size_t cx1, cy1;
  mpui_size_t cx2, cy2;
  int dup;
};

struct mpui_fonts {
  mpui_font_t *deflt;
  mpui_font_t **fonts;
};

#define MPUI_FONT_SIZE_DEFAULT -1

struct mpui_font {
  char *id;
  int size;
  mpui_color_t *color;
  mpui_color_t *focused_color;
  mpui_color_t *really_focused_color;
  font_desc_t *font_desc;
};

struct mpui_action {
  char *cmd;
  mpui_action_when_t when;
};

struct mpui_filetype {
  mpui_match_t match;
  mpui_image_t *icon;
  mpui_action_t **actions;
  char **exts;
  int dup;
};

struct mpui_filetypes {
  char *id;
  mpui_filetype_t **filetypes;
};

struct mpui_objects {
  mpui_object_t **objects;
};

struct mpui_object {
  char *id;
  int need_dup, dup;
  mpui_size_t x, y;
  mpui_element_t **elements;
  mpui_action_t **actions;
  mpui_keymaps_t *keymaps;
};

struct mpui_obj {
  mpui_container_t container;
  mpui_object_t *object;
};

struct mpui_menuitem {
  mpui_container_t container;
};

struct mpui_allmenuitem {
  mpui_container_t container;
  mpui_menu_t *menu;
};

struct mpui_menus {
  mpui_menu_t **menus;
};

struct mpui_menu {
  char *id;
  int is_browser;
  int is_playlist;
  mpui_orientation_t orientation, scrolling;
  mpui_size_t x, y, w, h;
  mpui_font_t *font;
  mpui_element_t **elements;
  mpui_keymaps_t *keymaps;
};

struct mpui_mnu {
  mpui_focus_box_t fb;
  mpui_menu_t *menu;
};

struct mpui_browser {
  mpui_menu_t menu;
  mpui_obj_t *border;
  mpui_allmenuitem_t *item_border;
  mpui_filetypes_t *filter;
  mpui_alignment_t align;
  mpui_size_t icon_w, icon_h;
  mpui_size_t item_w;
  mpui_size_t spacing;
  int cwd_id;
};

struct mpui_playlist {
  mpui_menu_t menu;
  mpui_obj_t *border;
  mpui_allmenuitem_t *item_border;
  mpui_alignment_t align;
  mpui_size_t item_w;
  mpui_size_t spacing;
  char **items;
  int need_generate;
  int focus_index;
};

struct mpui_infos {
  mpui_info_t **infos;
};

struct mpui_info {
  char *id;
  mpui_font_t *font;
  mpui_coord_t x, y, w, h;
  mpui_tag_t **tags;
  mpui_pic_t **pics;
  mpui_sys_t **sys;
};

struct mpui_inf {
  mpui_container_t container;
  mpui_info_t *info;
  char filename[NAME_MAX+1];
  int filename_id;
  int last_filename_id;
  unsigned int timer;
  unsigned int next_timer;
};

struct mpui_tag {
  char *id;
  char *caption;
  char *type;
  mpui_coord_t x, y;
};

struct mpui_pic {
  char *id;
  mpui_coord_t x, y, w, h;
  mpui_filetypes_t *filter;
};

struct mpui_sys {
  char *id;
  char *caption;
  char *type;
  mpui_coord_t x, y;
};

struct mpui_slideshow {
  mpui_container_t container;
  char path[NAME_MAX+1];
  char *name;
  int path_id;
  int last_path_id;
  mpui_filetypes_t *filter;
  mpui_slideshow_mode_t mode;
  int rotation;
  int play;
  unsigned int timer;
  unsigned int next_timer;
  mpui_coord_t name_x, name_y;
  mpui_font_t *name_font;
  mpui_obj_t *border;
  struct dirent **dirent;
  int dirent_size;
  int dirent_cur;
  int need_generate;
};

struct mpui_popups {
  mpui_popup_t **popups;
};

struct mpui_popup {
  mpui_container_t container;
  char *id;
};

struct mpui_screens {
  mpui_screen_t *menu;
  mpui_screen_t *ctrl;
  mpui_screen_t **screens;
};

struct mpui_screen {
  char *id;
  mpui_element_t **elements;
  mpui_element_t **focus_box;
  mpui_popup_t **popup_stack;
  mpui_keymaps_t *keymaps;
};

struct mpui {
  int width, height, diag;
  int format;
  char *theme;
  char *datadir;
  char *lang;
  mpui_keymapping_t *keymapping;
  mpui_strings_t *strings;
  mpui_fonts_t *fonts;
  mpui_images_t *images;
  mpui_filetypes_t **filetypes;
  mpui_objects_t *objects;
  mpui_menus_t *menus;
  mpui_infos_t *infos;
  mpui_popups_t *popups;
  mpui_screens_t *screens;
  mpui_screen_t *current_screen;
  char cwd[NAME_MAX+1];
  char *lwd;
  int cwd_id;
};


void *mpui_list_new (void);
int mpui_list_empty (void *list);
int mpui_list_length (void *list);
void *mpui_list_add (void *list, void *element);
void *mpui_list_remove_last (void *list);
void mpui_list_free (void *list, mpui_list_free_func_t func);

mpui_color_t *mpui_color_new (unsigned char r,unsigned char g,unsigned char b);
mpui_color_t *mpui_color_dup (mpui_color_t *color);
void mpui_color_free (mpui_color_t *color);

mpui_keymaps_t *mpui_keymaps_new (char *id);
int mpui_keymaps_add (mpui_keymaps_t *keymaps, char *key, char *cmd);
mpui_keymaps_t *mpui_keymaps_get (mpui_t *mpui, char *id);
void mpui_keymaps_free (mpui_keymaps_t *keymaps);

mpui_keymapping_t *mpui_keymapping_new (void);
void mpui_keymapping_free (mpui_keymapping_t *keymapping);

static inline void
mpui_keymapping_add (mpui_keymapping_t *keymapping, mpui_keymaps_t *keymaps)
{
  keymapping->keymaps = mpui_list_add (keymapping->keymaps, keymaps);
}

mpui_string_t *mpui_string_new (char *id, unsigned char *str,
                                mpui_encoding_t encoding);
mpui_string_t *mpui_string_get (mpui_t *mpui, char *id);
unsigned int mpui_string_get_next_char (unsigned char **txt,
                                        mpui_encoding_t encoding);
void mpui_string_free (mpui_string_t *string);

mpui_str_t *mpui_str_new (mpui_string_t *string, mpui_coord_t x,mpui_coord_t y,
                          mpui_flags_t flags, mpui_font_t *font, int size,
                          mpui_color_t *color, mpui_color_t *focused_color,
                          mpui_color_t *really_focused_color,
                          mpui_when_focused_t when_focused);
mpui_str_t *mpui_str_dup (mpui_str_t *str);
void mpui_str_free (mpui_str_t *str);

mpui_strings_t *mpui_strings_new (char *fonts, char *lang);
void mpui_strings_free (mpui_strings_t *strings);

static inline void
mpui_strings_add (mpui_strings_t *strings, mpui_string_t *string)
{
  strings->strings = mpui_list_add(strings->strings, string);
}

mpui_image_t *mpui_image_new (char *id, char *file,
                              mpui_size_t x, mpui_size_t y,
                              mpui_size_t w, mpui_size_t h);
mpui_image_t *mpui_image_get (mpui_t *mpui, char *id);
mpui_image_t *mpui_image_dup (mpui_image_t *image, mpui_size_t x,mpui_size_t y,
                              mpui_size_t w, mpui_size_t h);
void mpui_image_free (mpui_image_t *image);

mpui_img_t *mpui_img_new (mpui_image_t *image, mpui_coord_t x, mpui_coord_t y,
                          mpui_coord_t w, mpui_coord_t h, mpui_flags_t flags,
                          mpui_when_focused_t when_focused);
mpui_img_t *mpui_img_dup (mpui_img_t *img);
void mpui_img_load (mpui_t *mpui, mpui_img_t *img);
void mpui_img_free (mpui_img_t *img);

mpui_images_t *mpui_images_new (void);
void mpui_images_free (mpui_images_t *images);

static inline void
mpui_images_add (mpui_images_t *images, mpui_image_t *image)
{
  images->images = mpui_list_add(images->images, image);
}

mpui_font_t *mpui_font_new (mpui_t *mpui, char *id, char *file, int size,
                            mpui_color_t *color, mpui_color_t *focused_color,
                            mpui_color_t *really_focused_color);
mpui_font_t *mpui_font_get (mpui_t *mpui, char *id);
void mpui_font_free (mpui_font_t *font);

mpui_fonts_t *mpui_fonts_new (void);
void mpui_fonts_free (mpui_fonts_t *fonts);

static inline void
mpui_fonts_add (mpui_fonts_t *fonts, mpui_font_t *font)
{
  fonts->fonts = mpui_list_add(fonts->fonts, font);
}

mpui_filetype_t *mpui_filetype_new (mpui_match_t match);
mpui_filetype_t *mpui_filetype_dup (mpui_filetype_t *filetype,
                                    mpui_size_t icon_w, mpui_size_t icon_h);
void mpui_filetype_free (mpui_filetype_t *filetype);

static inline void
mpui_filetype_exts_add (mpui_filetype_t *filetype, char *ext)
{
  filetype->exts = mpui_list_add(filetype->exts, ext);
}

static inline void
mpui_filetype_actions_add (mpui_filetype_t *filetype, mpui_action_t *action)
{
  filetype->actions = mpui_list_add(filetype->actions, action);
}

mpui_filetypes_t *mpui_filetypes_new (char *id);
mpui_filetypes_t *mpui_filetypes_get (mpui_t *mpui, char *id);
mpui_filetypes_t *mpui_filetypes_dup (mpui_filetypes_t *filetypes,
                                      mpui_size_t icon_w, mpui_size_t icon_h);
void mpui_filetypes_free (mpui_filetypes_t *filetypes);

static inline void
mpui_filetypes_add (mpui_filetypes_t *filetypes, mpui_filetype_t *filetype)
{
  filetypes->filetypes = mpui_list_add(filetypes->filetypes, filetype);
}

static inline void
mpui_filetypes_list_add (mpui_t *mpui, mpui_filetypes_t *filetypes)
{
  mpui->filetypes = mpui_list_add(mpui->filetypes, filetypes);
}

mpui_object_t *mpui_object_new (char *id, mpui_size_t x, mpui_size_t y,
                                mpui_keymaps_t *keymaps);
mpui_object_t *mpui_object_get (mpui_t *mpui, char *id);
mpui_object_t *mpui_object_dup (mpui_object_t *object);
void mpui_object_free (mpui_object_t *object);

static inline void
mpui_object_actions_add (mpui_object_t *object, mpui_action_t *action)
{
  object->actions = mpui_list_add(object->actions, action);
}

static inline void
mpui_object_elements_add (mpui_object_t *object, mpui_element_t *element)
{
  object->elements = mpui_list_add(object->elements, element);
}

mpui_obj_t *mpui_obj_new (mpui_object_t *object, mpui_coord_t x,mpui_coord_t y,
                          mpui_flags_t flags,mpui_when_focused_t when_focused);
mpui_obj_t *mpui_obj_dup (mpui_obj_t *obj);
void mpui_obj_free (mpui_obj_t *obj);

mpui_objects_t *mpui_objects_new (void);
void mpui_objects_free (mpui_objects_t *objects);

static inline void
mpui_objects_add (mpui_objects_t *objects, mpui_object_t *object)
{
  objects->objects = mpui_list_add(objects->objects, object);
}

mpui_menu_t *mpui_menu_new (char * id, mpui_orientation_t orientation,
                            mpui_size_t x, mpui_size_t y, mpui_font_t *font,
                            mpui_keymaps_t *keymaps);
mpui_menu_t *mpui_menu_get (mpui_t *mpui, char *id);
void mpui_menu_free (mpui_menu_t *menu);

static inline void
mpui_menu_elements_add (mpui_menu_t *menu, mpui_element_t *element)
{
  menu->elements = mpui_list_add(menu->elements, element);
}

mpui_mnu_t *mpui_mnu_new (mpui_menu_t *menu, mpui_coord_t x, mpui_coord_t y,
                          mpui_flags_t flags);
void mpui_mnu_free (mpui_mnu_t *mnu);

mpui_menus_t *mpui_menus_new (void);
void mpui_menus_free (mpui_menus_t *menus);

static inline void
mpui_menus_add (mpui_menus_t *menus, mpui_menu_t *menu)
{
  menus->menus = mpui_list_add(menus->menus, menu);
}

mpui_menuitem_t *mpui_menuitem_new (void);
void mpui_menuitem_free (mpui_menuitem_t *menuitem);

mpui_allmenuitem_t *mpui_allmenuitem_new (mpui_menu_t *menu);
void mpui_allmenuitem_free (mpui_allmenuitem_t *allmenuitem);

mpui_action_t *mpui_action_new (char *cmd, mpui_action_when_t when);
void mpui_action_free (mpui_action_t *action);
void mpui_actions_free (mpui_action_t **actions);

mpui_element_t *mpui_element_dup (mpui_element_t *element);
void mpui_element_coord_dup (mpui_element_t *element);
void mpui_element_free (mpui_element_t *element);
void mpui_elements_get_size (mpui_element_t *element,
                             mpui_element_t **elements);
void mpui_elements_free (mpui_element_t **elements);

mpui_browser_t *mpui_browser_new (char * id, mpui_font_t *font,
                                  mpui_orientation_t orientation,
                                  mpui_orientation_t scrolling,
                                  mpui_alignment_t align,
                                  mpui_size_t x, mpui_size_t y,
                                  mpui_size_t w, mpui_size_t h,
                                  mpui_size_t icon_w, mpui_size_t icon_h,
                                  mpui_size_t item_w, mpui_size_t spacing,
                                  mpui_object_t *border,
                                  mpui_object_t *item_border,
                                  mpui_filetypes_t *filter,
                                  mpui_keymaps_t *keymaps);
void mpui_browser_free (mpui_browser_t *browser);

mpui_playlist_t *mpui_playlist_new (char * id, mpui_font_t *font,
                                    mpui_orientation_t orientation,
                                    mpui_orientation_t scrolling,
                                    mpui_alignment_t align,
                                    mpui_size_t x, mpui_size_t y,
                                    mpui_size_t w, mpui_size_t h,
                                    mpui_size_t item_w, mpui_size_t spacing,
                                    mpui_object_t *border,
                                    mpui_object_t *item_border,
                                    mpui_keymaps_t *keymaps);
void mpui_playlist_free (mpui_playlist_t *playlist);

mpui_tag_t *mpui_tag_new (char *id, char *caption, char *type,
                          mpui_coord_t x, mpui_coord_t y);
void mpui_tag_free (mpui_tag_t *tag);

mpui_pic_t *mpui_pic_new (char *id, mpui_coord_t x, mpui_coord_t y,
                          mpui_coord_t w, mpui_coord_t h,
                          mpui_filetypes_t *filter);
void mpui_pic_free (mpui_pic_t *pic);

mpui_sys_t *mpui_sys_new (char *id, char *caption, char *type,
                          mpui_coord_t x, mpui_coord_t y);
void mpui_sys_free (mpui_sys_t *sys);

mpui_info_t *mpui_info_new (char *id, mpui_font_t *font,
                            mpui_coord_t x, mpui_coord_t y,
                            mpui_coord_t w, mpui_coord_t h);
mpui_info_t *mpui_info_get (mpui_t *mpui, char *id);
void mpui_info_free (mpui_info_t *info);

static inline void
mpui_info_add_tag (mpui_info_t *info, mpui_tag_t *tag)
{
  info->tags = mpui_list_add(info->tags, tag);
}

static inline void
mpui_info_add_pic (mpui_info_t *info, mpui_pic_t *pic)
{
  info->pics = mpui_list_add(info->pics, pic);
}

static inline void
mpui_info_add_sys (mpui_info_t *info, mpui_sys_t *sys)
{
  info->sys = mpui_list_add(info->sys, sys);
}

mpui_inf_t *mpui_inf_new (mpui_info_t *info,
                          mpui_coord_t x, mpui_coord_t y,
                          mpui_when_focused_t when_focused,
                          unsigned int timer);
void mpui_inf_free (mpui_inf_t *inf);

mpui_infos_t *mpui_infos_new (void);
void mpui_infos_free (mpui_infos_t *infos);

static inline void
mpui_infos_add (mpui_infos_t *infos, mpui_info_t *info)
{
  infos->infos = mpui_list_add(infos->infos, info);
}

mpui_slideshow_t *mpui_slideshow_new (char *id, mpui_coord_t x, mpui_coord_t y,
                                      mpui_coord_t w, mpui_coord_t h,
                                      char *path, mpui_filetypes_t *filter,
                                      char *mode, int timer,
                                      mpui_coord_t name_x, mpui_coord_t name_y,
                                      mpui_font_t *name_font,
                                      mpui_object_t *border);
void mpui_slideshow_free (mpui_slideshow_t *slideshow);

mpui_popup_t *mpui_popup_new (char *id, mpui_coord_t x, mpui_coord_t y,
                              mpui_keymaps_t *keymaps);
mpui_popup_t *mpui_popup_get (mpui_popups_t *popups, char *id);
void mpui_popup_free (mpui_popup_t *popup);

mpui_popups_t *mpui_popups_new (void);
void mpui_popups_free (mpui_popups_t *popups);

static inline void
mpui_popups_add (mpui_popups_t *popups, mpui_popup_t *popup)
{
  popups->popups = mpui_list_add(popups->popups, popup);
}

mpui_screen_t *mpui_screen_new (char *id, mpui_keymaps_t *keymaps);
mpui_screen_t *mpui_screen_get (mpui_screens_t *screens, char *id);
void mpui_screen_free (mpui_screen_t *screen);

static inline void
mpui_screen_elements_add (mpui_screen_t *screen, mpui_element_t *element)
{
  screen->elements = mpui_list_add(screen->elements, element);
}

static inline void
mpui_screen_popups_add (mpui_screen_t *screen, mpui_popup_t *popup)
{
  screen->popup_stack = mpui_list_add(screen->popup_stack, popup);
}

static inline void *
mpui_screen_popups_remove_last (mpui_screen_t *screen)
{
  return mpui_list_remove_last (screen->popup_stack);
}

static inline int
mpui_screen_has_popups (mpui_screen_t *screen)
{
  return !mpui_list_empty (screen->popup_stack);
}

static inline mpui_popup_t *
mpui_screen_last_popup (mpui_screen_t *screen)
{
  return screen->popup_stack[mpui_list_length (screen->popup_stack) - 1];
}

mpui_screens_t *mpui_screens_new (void);
void mpui_screens_free (mpui_screens_t *screens);

static inline void
mpui_screens_add (mpui_screens_t *screens, mpui_screen_t *screen)
{
  screens->screens = mpui_list_add(screens->screens, screen);
}

mpui_t *mpui_new (int width, int height, int format, char *theme, char *lang);
void mpui_free (mpui_t *mpui);

static inline void
mpui_container_actions_add (mpui_container_t *container, mpui_action_t *action)
{
  container->actions = mpui_list_add(container->actions, action);
}

static inline void
mpui_container_elements_add (mpui_container_t *container, mpui_element_t *element)
{
  container->elements = mpui_list_add(container->elements, element);
}

#endif  /* MPUI_STRUCT_H */
