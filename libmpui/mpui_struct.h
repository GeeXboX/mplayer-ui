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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MPUI_STRUCT_H
#define MPUI_STRUCT_H

#include <stdlib.h>
#include "config.h"
#include "libvo/font_load.h"


typedef enum mpui_when_focused mpui_when_focused_t;
typedef enum mpui_type mpui_type_t;
typedef struct mpui_element mpui_element_t;
typedef struct mpui_container mpui_container_t;
typedef struct mpui_focus_box mpui_focus_box_t;
typedef int mpui_size_t;
typedef struct mpui_coord mpui_coord_t;
typedef struct mpui_color mpui_color_t;
typedef struct mpui_ids mpui_ids_t;

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
typedef unsigned int mpui_flags_t;
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
typedef struct mpui_infos mpui_infos_t;
typedef struct mpui_info mpui_info_t;
typedef struct mpui_inf mpui_inf_t;
typedef struct mpui_tag mpui_tag_t;
typedef struct mpui_popup mpui_popup_t;
typedef struct mpui_popups mpui_popups_t;
typedef struct mpui_screens mpui_screens_t;
typedef struct mpui_screen mpui_screen_t;
typedef struct mpui mpui_t;

#define MPUI_FLAG_ABSOLUTE  ((mpui_flags_t) 0x01)
#define MPUI_FLAG_DYNAMIC   ((mpui_flags_t) 0x02)
#define MPUI_FLAG_NOCOORD   ((mpui_flags_t) 0x04)
#define MPUI_FLAG_CONTAINER ((mpui_flags_t) 0x08)
#define MPUI_FLAG_FOCUS_BOX ((mpui_flags_t) 0x10)
#define MPUI_FLAG_FOCUSABLE ((mpui_flags_t) 0x20)
#define MPUI_FLAG_OWNER     ((mpui_flags_t) 0x40)
#define MPUI_FLAG_HIDDEN    ((mpui_flags_t) 0x80)

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
  mpui_orientation_t orientation, scrolling;
  mpui_size_t x, y, w, h;
  mpui_element_t **elements;
};

struct mpui_mnu {
  mpui_focus_box_t fb;
  mpui_menu_t *menu;
};

struct mpui_browser {
  mpui_menu_t menu;
  mpui_font_t *font;
  mpui_obj_t *border;
  mpui_allmenuitem_t *item_border;
  mpui_filetypes_t *filter;
  mpui_alignment_t align;
  mpui_size_t icon_w, icon_h;
  mpui_size_t item_w;
  mpui_size_t spacing;
  int cwd_id;
};

struct mpui_infos {
  mpui_info_t **info;
};

struct mpui_info {
  char *id;
  mpui_font_t *font;
  mpui_coord_t x, y, w, h;
  mpui_tag_t **tags;
};

struct mpui_inf {
  mpui_container_t container;
  mpui_info_t *info;
};

struct mpui_tag {
  char *id;
  char *caption;
  char *type;
  mpui_coord_t x, y;
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
};

struct mpui {
  int width, height, diag;
  int format;
  char *theme;
  char *datadir;
  char *lang;
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
  mpui_screen_t *previous_screen;
  char cwd[NAME_MAX+1];
  char *lwd;
  int cwd_id;
};


void *mpui_list_new (void);
int mpui_list_empty (void *list);
int mpui_list_length (void *list);
void *mpui_list_add (void *list, void *element);
void mpui_list_remove_last (void *list);

mpui_color_t *mpui_color_new (unsigned char r,unsigned char g,unsigned char b);
mpui_color_t *mpui_color_dup (mpui_color_t *color);
void mpui_color_free (mpui_color_t *color);

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
#define mpui_strings_add(a,b) (a)->strings = mpui_list_add((a)->strings, (b))
void mpui_strings_free (mpui_strings_t *strings);

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
#define mpui_images_add(a,b) (a)->images = mpui_list_add((a)->images, (b))
void mpui_images_free (mpui_images_t *images);

mpui_font_t *mpui_font_new (mpui_t *mpui, char *id, char *file, int size,
                            mpui_color_t *color, mpui_color_t *focused_color,
                            mpui_color_t *really_focused_color);
mpui_font_t *mpui_font_get (mpui_t *mpui, char *id);
void mpui_font_free (mpui_font_t *font);

mpui_fonts_t *mpui_fonts_new (void);
#define mpui_fonts_add(a,b) (a)->fonts = mpui_list_add((a)->fonts, (b))
void mpui_fonts_free (mpui_fonts_t *fonts);

mpui_filetype_t *mpui_filetype_new (mpui_match_t match);
#define mpui_ext_add(a,b) (a)->exts = mpui_list_add((a)->exts, (b))
mpui_filetype_t *mpui_filetype_dup (mpui_filetype_t *filetype,
                                    mpui_size_t icon_w, mpui_size_t icon_h);
void mpui_filetype_free (mpui_filetype_t *filetype);
mpui_filetypes_t *mpui_filetypes_new (char *id);
#define mpui_filetypes_add(a,b) (a)->filetypes = mpui_list_add((a)->filetypes, (b))
mpui_filetypes_t *mpui_filetypes_get (mpui_t *mpui, char *id);
mpui_filetypes_t *mpui_filetypes_dup (mpui_filetypes_t *filetypes,
                                      mpui_size_t icon_w, mpui_size_t icon_h);
void mpui_filetypes_free (mpui_filetypes_t *filetypes);

mpui_object_t *mpui_object_new (char *id, mpui_size_t x, mpui_size_t y);
mpui_object_t *mpui_object_get (mpui_t *mpui, char *id);
mpui_object_t *mpui_object_dup (mpui_object_t *object);
void mpui_object_free (mpui_object_t *object);

mpui_obj_t *mpui_obj_new (mpui_object_t *object, mpui_coord_t x,mpui_coord_t y,
                          mpui_flags_t flags,mpui_when_focused_t when_focused);
mpui_obj_t *mpui_obj_dup (mpui_obj_t *obj);
void mpui_obj_free (mpui_obj_t *obj);

mpui_objects_t *mpui_objects_new (void);
#define mpui_objects_add(a,b) (a)->objects = mpui_list_add((a)->objects, (b))
void mpui_objects_free (mpui_objects_t *objects);

mpui_menu_t *mpui_menu_new (char * id, mpui_orientation_t orientation,
                            mpui_size_t x, mpui_size_t y);
mpui_menu_t *mpui_menu_get (mpui_t *mpui, char *id);
void mpui_menu_free (mpui_menu_t *menu);

mpui_mnu_t *mpui_mnu_new (mpui_menu_t *menu, mpui_coord_t x, mpui_coord_t y,
                          mpui_flags_t flags);
void mpui_mnu_free (mpui_mnu_t *mnu);

mpui_menus_t *mpui_menus_new (void);
#define mpui_menus_add(a,b) (a)->menus = mpui_list_add((a)->menus, (b))
void mpui_menus_free (mpui_menus_t *menus);

mpui_menuitem_t *mpui_menuitem_new (void);
void mpui_menuitem_free (mpui_menuitem_t *menuitem);

mpui_allmenuitem_t *mpui_allmenuitem_new (mpui_menu_t *menu);
void mpui_allmenuitem_free (mpui_allmenuitem_t *allmenuitem);

mpui_action_t *mpui_action_new (char *cmd, mpui_action_when_t when);
#define mpui_actions_add(a,b) (a)->actions = mpui_list_add((a)->actions, (b))
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
                                  mpui_filetypes_t *filter);
void mpui_browser_free (mpui_browser_t *browser);

mpui_tag_t *mpui_tag_new (char *id, char *caption, char *type,
                          mpui_coord_t x, mpui_coord_t y);
void mpui_tag_free (mpui_tag_t *tag);

mpui_info_t *mpui_info_new (char *id, mpui_font_t *font,
                            mpui_coord_t x, mpui_coord_t y,
                            mpui_coord_t w, mpui_coord_t h);
mpui_info_t *mpui_info_get (mpui_t *mpui, char *id);
#define mpui_info_add(a,b) (a)->tags = mpui_list_add((a)->tags, (b))
void mpui_info_free (mpui_info_t *info);

mpui_inf_t *mpui_inf_new (mpui_info_t *info,
                          mpui_coord_t x, mpui_coord_t y,
                          mpui_when_focused_t when_focused);
void mpui_inf_free (mpui_inf_t *inf);

mpui_infos_t *mpui_infos_new (void);
#define mpui_infos_add(a,b) (a)->info = mpui_list_add((a)->info, (b))
void mpui_infos_free (mpui_infos_t *infos);

mpui_popup_t *mpui_popup_new (char *id, mpui_coord_t x, mpui_coord_t y);
mpui_popup_t *mpui_popup_get (mpui_popups_t *popups, char *id);
void mpui_popup_free (mpui_popup_t *popup);

mpui_popups_t *mpui_popups_new (void);
#define mpui_popups_add(a,b) (a)->popups = mpui_list_add((a)->popups, (b))
void mpui_popups_free (mpui_popups_t *popups);

mpui_screen_t *mpui_screen_new (char *id);
mpui_screen_t *mpui_screen_get (mpui_screens_t *screens, char *id);
#define mpui_popup_add(a,b) (a)->popup_stack = mpui_list_add((a)->popup_stack, (b))
#define mpui_popup_remove(a) mpui_list_remove_last((a)->popup_stack)
void mpui_screen_free (mpui_screen_t *screen);

#define mpui_add_element(a,b) (a)->elements = mpui_list_add((a)->elements, (b))

mpui_screens_t *mpui_screens_new (void);
#define mpui_screens_add(a,b) (a)->screens = mpui_list_add((a)->screens, (b))
void mpui_screens_free (mpui_screens_t *screens);

mpui_t *mpui_new (int width, int height, int format, char *theme, char *lang);
void mpui_free (mpui_t *mpui);

#endif  /* MPUI_STRUCT_H */
