/*
 *  mpui_struct.c: libmpui structures storage.
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

#include <stdlib.h>
#include <string.h>

#include "mpui_struct.h"


static inline char *
mpui_strdup (char *str)
{
  if (!str)
    return str;
  return strdup (str);
}

void *
mpui_list_new (void)
{
  void **list;

  list = (void **) malloc (sizeof (*list));
  *list = NULL;
  return list;
}

int
mpui_list_length (void *list)
{
  void **l = list;
  int n = 0;
  while (*l++)
    n++;
  return n;
}

void *
mpui_list_add (void *list, void *element)
{
  void **l = list;
  int n = mpui_list_length (list) + 1;
  l = (void **) realloc (l, (n+1) * sizeof (*l));
  l[n] = NULL;
  l[n-1] = element;
  return l;
}


mpui_string_t *
mpui_string_new (char *id, char *str)
{
  mpui_string_t *string;

  string = (mpui_string_t *) malloc (sizeof (*string));
  string->id = mpui_strdup (id);
  string->text = mpui_strdup (str);
  return string;
}

mpui_string_t *
mpui_string_get (mpui_t *mpui, char *id)
{
  mpui_strings_t **strings;
  mpui_string_t **string;

  for (strings=mpui->strings; *strings; strings++)
    for (string=(*strings)->strings; *string; string++)
      if (!strcmp ((*string)->id, id))
        return *string;
  return NULL;
}

void
mpui_string_free (mpui_string_t *string)
{
  free (string->id);
  free (string->text);
  free (string);
}


mpui_str_t *
mpui_str_new (mpui_string_t *string, mpui_size_t x, mpui_size_t y,
              mpui_font_t *font, int size, mpui_color_t color,
              mpui_color_t focused_color, mpui_when_focused_t when_focused)
{
  mpui_str_t *str;

  str = (mpui_str_t *) malloc (sizeof (*str));
  str->string = string;
  str->x = x;
  str->y = y;
  str->font = font;
  str->size = size;
  str->color = color;
  str->focused_color = focused_color;
  str->when_focused = when_focused;
  return str;
}

void
mpui_str_free (mpui_str_t *str)
{
  free (str);
}


mpui_strings_t *
mpui_strings_new (char *encoding, char *lang)
{
  mpui_strings_t *strings;

  strings = (mpui_strings_t *) malloc (sizeof (*strings));
  strings->encoding = mpui_strdup (encoding);
  strings->lang = mpui_strdup (lang);
  strings->strings = mpui_list_new ();
  return strings;
}

void
mpui_strings_free (mpui_strings_t *strings)
{
  mpui_string_t **s = strings->strings;

  free (strings->encoding);
  free (strings->lang);
  while (*s)
    mpui_string_free (*s++);
  free (strings->strings);
  free (strings);
}


mpui_image_t *
mpui_image_new (char *id, char *file, mpui_size_t x, mpui_size_t y,
                mpui_size_t h, mpui_size_t w)
{
  mpui_image_t *image;

  image = (mpui_image_t *) malloc (sizeof (*image));
  image->id = mpui_strdup (id);
  image->file = mpui_strdup (file);
  image->x = x;
  image->y = y;
  image->h = h;
  image->w = w;

  return image;
}

mpui_image_t *
mpui_image_get (mpui_t *mpui, char *id)
{
  mpui_images_t **images;
  mpui_image_t **image;

  for (images=mpui->images; *images; images++)
    for (image=(*images)->images; *image; image++)
      if (!strcmp ((*image)->id, id))
        return *image;
  return NULL;
}

void
mpui_image_free (mpui_image_t *image)
{
  free (image->id);
  free (image->file);
  free (image);
}

mpui_img_t *
mpui_img_new (mpui_image_t *image, mpui_size_t x, mpui_size_t y,
              mpui_size_t h, mpui_size_t w, mpui_when_focused_t when_focused)
{
  mpui_img_t *img;

  img = (mpui_img_t *) malloc (sizeof (*img));
  img->image = image;
  img->x = x;
  img->y = y;
  img->h = h;
  img->w = w;
  img->when_focused = when_focused;

  return img;
}

void
mpui_img_free (mpui_img_t *img)
{
  free (img);
}

mpui_images_t *
mpui_images_new (void)
{
  mpui_images_t *images;

  images = (mpui_images_t *) malloc (sizeof (*images));
  images->images = mpui_list_new ();
  return images;
}

void
mpui_images_free (mpui_images_t *images)
{
  mpui_image_t **i = images->images;

  while (*i)
    mpui_image_free (*i++);
  free (images->images);
  free (images);
}


mpui_font_t *
mpui_font_new (char *id, char *file, int size,
               mpui_color_t color, mpui_color_t focused_color)
{
  mpui_font_t *font;

  font = (mpui_font_t *) malloc (sizeof (*font));
  font->id = mpui_strdup (id);
  font->file = mpui_strdup (file);
  font->size = size;
  font->color = color;
  font->focused_color = focused_color;

  return font;
}

mpui_font_t *
mpui_font_get (mpui_t *mpui, char *id)
{
  mpui_fonts_t **fonts;
  mpui_font_t **font;

  for (fonts=mpui->fonts; *fonts; fonts++)
    for (font=(*fonts)->fonts; *font; font++)
      if (!strcmp ((*font)->id, id))
        return *font;
  return NULL;
}

void
mpui_font_free (mpui_font_t *font)
{
  free (font->id);
  free (font->file);
  free (font);
}


mpui_fonts_t *
mpui_fonts_new (void)
{
  mpui_fonts_t *fonts;

  fonts = (mpui_fonts_t *) malloc (sizeof (*fonts));
  fonts->fonts = mpui_list_new ();
  return fonts;
}

void
mpui_fonts_free (mpui_fonts_t *fonts)
{
  mpui_font_t **f = fonts->fonts;

  while (*f)
    mpui_font_free (*f++);
  free (fonts->fonts);
  free (fonts);
}


mpui_object_t *
mpui_object_new (char *id, mpui_object_flags_t flags)
{
  mpui_object_t *object;

  object = (mpui_object_t *) malloc (sizeof (*object));
  object->id = mpui_strdup (id);
  object->flags = flags;
  object->elements = mpui_list_new ();
  return object;
}

mpui_object_t *
mpui_object_get (mpui_t *mpui, char *id)
{
  mpui_objects_t **objects;
  mpui_object_t **object;

  for (objects=mpui->objects; *objects; objects++)
    for (object=(*objects)->objects; *object; object++)
      if (!strcmp ((*object)->id, id))
        return *object;
  return NULL;
}

void
mpui_object_free (mpui_object_t *object)
{
  mpui_element_t **e = object->elements;

  while (*e)
    mpui_element_free (*e++);
  free (object->elements);
  free (object->id);
  free (object);
}

mpui_obj_t *
mpui_obj_new (mpui_object_t *object, mpui_when_focused_t when_focused)
{
  mpui_obj_t *obj;

  obj = (mpui_obj_t *) malloc (sizeof (*obj));
  obj->object = object;
  obj->when_focused = when_focused;

  return obj;
}

void
mpui_obj_free (mpui_obj_t *obj)
{
  free (obj);
}

mpui_objects_t *
mpui_objects_new (void)
{
  mpui_objects_t *objects;

  objects = (mpui_objects_t *) malloc (sizeof (*objects));
  objects->objects = mpui_list_new ();
  return objects;
}

void
mpui_objects_free (mpui_objects_t *objects)
{
  mpui_object_t **o = objects->objects;

  while (*o)
    mpui_object_free (*o++);
  free (objects->objects);
  free (objects);
}


mpui_menu_t *
mpui_menu_new (mpui_menu_orientation_t orientation, mpui_font_t *font,
               mpui_size_t x, mpui_size_t y, mpui_size_t w, mpui_size_t h)
{
  mpui_menu_t *menu;

  menu = (mpui_menu_t *) malloc (sizeof (*menu));
  menu->orientation = orientation;
  menu->font = font;
  menu->x = x;
  menu->y = y;
  menu->w = w;
  menu->h = h;
  menu->elements = mpui_list_new ();
  return menu;
}

void
mpui_menu_free (mpui_menu_t *menu)
{
  mpui_element_t **e = menu->elements;

  while (*e)
    mpui_element_free (*e++);
  free (menu->elements);
  free (menu);
}


mpui_menus_t *
mpui_menus_new (void)
{
  mpui_menus_t *menus;

  menus = (mpui_menus_t *) malloc (sizeof (*menus));
  menus->menus = mpui_list_new ();
  return menus;
}

void
mpui_menus_free (mpui_menus_t *menus)
{
  mpui_menu_t **m = menus->menus;

  while (*m)
    mpui_menu_free (*m++);
  free (menus->menus);
  free (menus);
}


mpui_menuitem_t *
mpui_menuitem_new (void)
{
  mpui_menuitem_t *menuitem;

  menuitem = (mpui_menuitem_t *) malloc (sizeof (*menuitem));
  menuitem->elements = mpui_list_new ();
  return menuitem;
}

void
mpui_menuitem_free (mpui_menuitem_t *menuitem)
{
  mpui_element_t **e = menuitem->elements;

  while (*e)
    mpui_element_free (*e++);
  free (menuitem->elements);
  free (menuitem);
}


mpui_action_t *
mpui_action_new (char *cmd)
{
  mpui_action_t *action;

  action = (mpui_action_t *) malloc (sizeof (*action));
  action->cmd = mpui_strdup (cmd);
  return action;
}

void
mpui_action_free (mpui_action_t *action)
{
  free (action->cmd);
  free (action);
}


mpui_element_t *
mpui_element_new (mpui_type_t type, void *elem)
{
  mpui_element_t *element;

  element = (mpui_element_t *) malloc (sizeof (*element));
  element->type = type;
  element->ptr = elem;
  return element;
}

void
mpui_element_free (mpui_element_t *element)
{
  if (element->type == MPUI_ACTION)
    mpui_action_free (element->action);
  free (element);
}


mpui_screen_t *
mpui_screen_new (char *id)
{
  mpui_screen_t *screen;

  screen = (mpui_screen_t *) malloc (sizeof (*screen));
  screen->id = mpui_strdup (id);
  screen->elements = mpui_list_new ();
  return screen;
}

mpui_screen_t *
mpui_screen_get (mpui_t *mpui, char *id)
{
  mpui_screens_t **screens;
  mpui_screen_t **screen;

  for (screens=mpui->screens; *screens; screens++)
    for (screen=(*screens)->screens; *screen; screen++)
      if (!strcmp ((*screen)->id, id))
        return *screen;
  return NULL;
}

void
mpui_screen_free (mpui_screen_t *screen)
{
  mpui_element_t **e = screen->elements;

  while (*e)
    mpui_element_free (*e++);
  free (screen->elements);
  free (screen);
}


mpui_screens_t *
mpui_screens_new (void)
{
  mpui_screens_t *screens;

  screens = (mpui_screens_t *) malloc (sizeof (*screens));
  screens->screens = mpui_list_new ();
  return screens;
}

void
mpui_screens_free (mpui_screens_t *screens)
{
  mpui_screen_t **s = screens->screens;

  while (*s)
    mpui_screen_free (*s++);
  free (screens->screens);
  free (screens);
}

mpui_t *
mpui_new (void)
{
  mpui_t *mpui;

  mpui = (mpui_t *) malloc (sizeof (*mpui));
  mpui->strings = mpui_list_new ();
  mpui->images = mpui_list_new ();
  mpui->fonts = mpui_list_new ();
  mpui->objects = mpui_list_new ();
  mpui->menus = mpui_list_new ();
  mpui->screens = mpui_list_new ();
  return mpui;
}

void
mpui_free (mpui_t *mpui)
{
  mpui_strings_t **strings = mpui->strings;
  mpui_images_t **images = mpui->images;
  mpui_fonts_t **fonts = mpui->fonts;
  mpui_objects_t **objects = mpui->objects;
  mpui_menus_t **menus = mpui->menus;
  mpui_screens_t **screens = mpui->screens;

  while (*strings)
    mpui_strings_free (*strings++);
  free (mpui->strings);

  while (*images)
    mpui_images_free (*images++);
  free (mpui->images);

  while (*fonts)
    mpui_fonts_free (*fonts++);
  free (mpui->fonts);

  while (*objects)
    mpui_objects_free (*objects++);
  free (mpui->objects);

  while (*menus)
    mpui_menus_free (*menus++);
  free (mpui->menus);

  while (*screens)
    mpui_screens_free (*screens++);
  free (mpui->screens);
}
