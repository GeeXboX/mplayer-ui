/*
 *  mpui_struct.h: libmpui structures storage.
 *  Copyright (C) 2004  Aurelien Jacobs
 *  Copyright (C) 2004  Benjamin Zores
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
#include "../libvo/font_load.h"


typedef enum mpui_when_focused mpui_when_focused_t;
typedef enum mpui_type mpui_type_t;
typedef struct mpui_element mpui_element_t;
typedef struct mpui_focus_box mpui_focus_box_t;
typedef int mpui_size_t;
typedef struct mpui_color mpui_color_t;
typedef struct mpui_ids mpui_ids_t;

typedef struct mpui_strings mpui_strings_t;
typedef struct mpui_string mpui_string_t;
typedef struct mpui_str mpui_str_t;
typedef struct mpui_images mpui_images_t;
typedef struct mpui_image mpui_image_t;
typedef struct mpui_img mpui_img_t;
typedef struct mpui_fonts mpui_fonts_t;
typedef struct mpui_font mpui_font_t;
typedef struct mpui_action mpui_action_t;
typedef unsigned int mpui_flags_t;
typedef struct mpui_objects mpui_objects_t;
typedef struct mpui_object mpui_object_t;
typedef struct mpui_obj mpui_obj_t;
typedef enum mpui_orientation mpui_orientation_t;
typedef struct mpui_menuitem mpui_menuitem_t;
typedef struct mpui_allmenuitem mpui_allmenuitem_t;
typedef struct mpui_menus mpui_menus_t;
typedef struct mpui_menu mpui_menu_t;
typedef struct mpui_mnu mpui_mnu_t;
typedef struct mpui_screens mpui_screens_t;
typedef struct mpui_screen mpui_screen_t;
typedef struct mpui mpui_t;

#define MPUI_FLAG_ABSOLUTE  ((mpui_flags_t) 0x01)
#define MPUI_FLAG_DYNAMIC   ((mpui_flags_t) 0x02)
#define MPUI_FLAG_NOCOORD   ((mpui_flags_t) 0x04)
#define MPUI_FLAG_FOCUS_BOX ((mpui_flags_t) 0x08)
#define MPUI_FLAG_FOCUSABLE ((mpui_flags_t) 0x10)

enum mpui_when_focused {
  MPUI_DISPLAY_NORMAL,
  MPUI_DISPLAY_REALLY_NORMAL,
  MPUI_DISPLAY_FOCUSED,
  MPUI_DISPLAY_REALLY_FOCUSED,
  MPUI_DISPLAY_ALWAYS,
};

enum mpui_orientation {
  MPUI_ORIENTATION_H,
  MPUI_ORIENTATION_V,
};

enum mpui_type {
  MPUI_STR,
  MPUI_IMG,
  MPUI_OBJ,
  MPUI_MNU,
  MPUI_MENUITEM,
};

struct mpui_element {
  mpui_type_t type;
  mpui_flags_t flags;
  mpui_size_t x, y;
  mpui_size_t w, h;
  mpui_when_focused_t when_focused;
  char *sx, *sy;
  char *sw, *sh;
};

struct mpui_focus_box {
  mpui_element_t element;
  mpui_element_t **elements;
  mpui_element_t **focus;
  mpui_orientation_t orientation;
};

struct mpui_color {
  unsigned char r, g, b;
  unsigned char y, u, v;
};

struct mpui_strings {
  char *encoding;
  char *lang;
  mpui_string_t **strings;
};

struct mpui_string {
  char *id;
  char *text;
};

struct mpui_str {
  mpui_element_t element;
  mpui_string_t *string;
  mpui_font_t *font;
  int size;
  mpui_color_t *color;
  mpui_color_t *focused_color;
};

struct mpui_images {
  mpui_image_t **images;
};

struct mpui_image {
  char *id;
  char *file;
  mpui_size_t x, y, w, h;
  int format;         /* one of the IMGFMT_* */
  int width, height;  /* original size of image file */
  int alpha;          /* does the image contain an alpha channel */
  int bpp;            /* byte per pixel (only for packed format) */
  int num_planes;     /* 1 for packed format, generaly 3 for planar */
  unsigned char *planes[5];
  unsigned int stride[5];
  int chroma_width, chroma_height;  /* only for planar formats */
};

struct mpui_img {
  mpui_element_t element;
  mpui_image_t *image;
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
  font_desc_t *font_desc;
};

struct mpui_action {
  char *cmd;
};

struct mpui_objects {
  mpui_object_t **objects;
};

struct mpui_object {
  char *id;
  mpui_size_t x, y;
  mpui_element_t **elements;
  mpui_action_t **actions;
};

struct mpui_obj {
  mpui_element_t element;
  mpui_object_t *object;
};

struct mpui_menuitem {
  mpui_element_t element;
  mpui_element_t **elements;
  mpui_action_t **actions;
};

struct mpui_allmenuitem {
  mpui_element_t **elements;
  mpui_action_t **actions;
};

struct mpui_menus {
  mpui_menu_t **menus;
};

struct mpui_menu {
  char *id;
  mpui_orientation_t orientation;
  mpui_size_t x, y, w, h;
  mpui_allmenuitem_t *allmenuitem;
  mpui_element_t **elements;
};

struct mpui_mnu {
  mpui_focus_box_t fb;
  mpui_menu_t *menu;
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
};

struct mpui {
  int width, height;
  int format;
  mpui_strings_t **strings;
  mpui_images_t **images;
  mpui_fonts_t **fonts;
  mpui_objects_t **objects;
  mpui_menus_t **menus;
  mpui_screens_t *screens;
};


void *mpui_list_new (void);
int mpui_list_length (void *list);
void *mpui_list_add (void *list, void *element);

mpui_color_t *mpui_color_new (unsigned char r,unsigned char g,unsigned char b);
void mpui_color_free (mpui_color_t *color);

mpui_string_t *mpui_string_new (char *id, char *str);
mpui_string_t *mpui_string_get (mpui_t *mpui, char *id);
void mpui_string_free (mpui_string_t *string);

mpui_str_t *mpui_str_new (mpui_string_t *string, mpui_size_t x, mpui_size_t y,
                          mpui_flags_t flags, mpui_font_t *font, int size,
                          mpui_color_t *color, mpui_color_t *focused_color,
                          mpui_when_focused_t when_focused);
void mpui_str_free (mpui_str_t *str);

mpui_strings_t *mpui_strings_new (char *encoding, char *lang);
#define mpui_strings_add(a,b) a->strings = mpui_list_add(a->strings, b)
void mpui_strings_free (mpui_strings_t *strings);

mpui_image_t *mpui_image_new (char *id, char *file,
                              mpui_size_t x, mpui_size_t y,
                              mpui_size_t w, mpui_size_t h);
mpui_image_t *mpui_image_get (mpui_t *mpui, char *id);
void mpui_image_free (mpui_image_t *image);

mpui_img_t *mpui_img_new (mpui_image_t *image, mpui_size_t x, mpui_size_t y,
                          mpui_size_t w, mpui_size_t h, mpui_flags_t flags,
                          mpui_when_focused_t when_focused);
void mpui_img_load (mpui_t *mpui, mpui_img_t *img);
void mpui_img_free (mpui_img_t *img);

mpui_images_t *mpui_images_new (void);
#define mpui_images_add(a,b) a->images = mpui_list_add(a->images, b)
void mpui_images_free (mpui_images_t *images);

mpui_font_t *mpui_font_new (mpui_t *mpui, char *id, char *file, int size,
                            mpui_color_t *color, mpui_color_t *focused_color);
mpui_font_t *mpui_font_get (mpui_t *mpui, char *id);
void mpui_font_free (mpui_font_t *font);

mpui_fonts_t *mpui_fonts_new (void);
#define mpui_fonts_add(a,b) a->fonts = mpui_list_add(a->fonts, b)
void mpui_fonts_free (mpui_fonts_t *fonts);

mpui_object_t *mpui_object_new (char *id, mpui_size_t x, mpui_size_t y);
mpui_object_t *mpui_object_get (mpui_t *mpui, char *id);
void mpui_object_free (mpui_object_t *object);

mpui_obj_t *mpui_obj_new (mpui_object_t *object, mpui_size_t x, mpui_size_t y,
                          mpui_flags_t flags,mpui_when_focused_t when_focused);
void mpui_obj_free (mpui_obj_t *obj);

mpui_objects_t *mpui_objects_new (void);
#define mpui_objects_add(a,b) a->objects = mpui_list_add(a->objects, b)
void mpui_objects_free (mpui_objects_t *objects);

mpui_menu_t *mpui_menu_new (char * id, mpui_orientation_t orientation,
                            mpui_size_t x, mpui_size_t y,
                            mpui_size_t w, mpui_size_t h);
mpui_menu_t *mpui_menu_get (mpui_t *mpui, char *id);
void mpui_menu_free (mpui_menu_t *menu);

mpui_mnu_t *mpui_mnu_new (mpui_menu_t *menu, mpui_size_t x, mpui_size_t y,
                          mpui_flags_t flags);
void mpui_mnu_free (mpui_mnu_t *mnu);

mpui_menus_t *mpui_menus_new (void);
#define mpui_menus_add(a,b) a->menus = mpui_list_add(a->menus, b)
void mpui_menus_free (mpui_menus_t *menus);

mpui_menuitem_t *mpui_menuitem_new (void);
void mpui_menuitem_free (mpui_menuitem_t *menuitem);

mpui_allmenuitem_t *mpui_allmenuitem_new (void);
void mpui_allmenuitem_free (mpui_allmenuitem_t *allmenuitem);

mpui_action_t *mpui_action_new (char *cmd);
#define mpui_actions_add(a,b) a->actions = mpui_list_add(a->actions, b)
void mpui_action_free (mpui_action_t *action);

void mpui_element_free (mpui_element_t *element);
void mpui_elements_get_size (mpui_element_t *element,
                             mpui_element_t **elements,
                             mpui_element_t **elements2);

mpui_screen_t *mpui_screen_new (char *id);
mpui_screen_t *mpui_screen_get (mpui_screens_t *screens, char *id);
/* mpui_screen_t *mpui_screen_get (mpui_t *mpui, char *id); */
void mpui_screen_free (mpui_screen_t *screen);

#define mpui_add_element(a,b) a->elements = mpui_list_add(a->elements, b)

mpui_screens_t *mpui_screens_new (void);
#define mpui_screens_add(a,b) a->screens = mpui_list_add(a->screens, b)
void mpui_screens_free (mpui_screens_t *screens);

mpui_t *mpui_new (int width, int height, int format);
void mpui_free (mpui_t *mpui);

#endif  /* MPUI_STRUCT_H */
