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
#include "mpui_focus.h"
#include "mpui_image.h"


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


#define mpui_rgb2y(R,G,B)  ((( 263*R + 516*G + 100*B) >> 10) + 16)
#define mpui_rgb2u(R,G,B)  (((-152*R - 298*G + 450*B) >> 10) + 128)
#define mpui_rgb2v(R,G,B)  ((( 450*R - 376*G -  73*B) >> 10) + 128)

mpui_color_t *
mpui_color_new (unsigned char r, unsigned char g, unsigned char b)
{
  mpui_color_t *color;

  color = (mpui_color_t *) malloc (sizeof (*color));
  color->r = r;
  color->g = g;
  color->b = b;
  color->y = mpui_rgb2y (color->r, color->g, color->b);
  color->u = mpui_rgb2u (color->r, color->g, color->b);
  color->v = mpui_rgb2v (color->r, color->g, color->b);
  return color;
}

void
mpui_color_free (mpui_color_t *color)
{
  free (color);
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


static void
mpui_str_get_size (mpui_str_t *str)
{
  font_desc_t *font = str->font->font_desc;
  char *s = str->string->text;
  int f, w = 0, h = 0;

  while (*s)
    {
      render_one_glyph (font, *s);
      f = font->font[(int)*s];

      w += font->width[(int)*s] + font->charspace;
      if (f >= 0 && font->pic_a[f]->h > h)
        h = font->pic_a[f]->h;

      s++;
    }

  str->element.w = w;
  str->element.h = h;
}

mpui_str_t *
mpui_str_new (mpui_string_t *string, mpui_size_t x, mpui_size_t y,
              mpui_flags_t flags, mpui_font_t *font, int size,
              mpui_color_t *color, mpui_color_t *focused_color,
              mpui_when_focused_t when_focused)
{
  mpui_str_t *str;

  str = (mpui_str_t *) malloc (sizeof (*str));
  str->element.type = MPUI_STR;
  str->element.flags = flags;
  str->element.x = x;
  str->element.y = y;
  str->element.when_focused = when_focused;
  str->string = string;
  str->font = font;
  str->size = size;
  str->color = color;
  str->focused_color = focused_color;
  mpui_str_get_size (str);
  return str;
}

void
mpui_str_free (mpui_str_t *str)
{
  mpui_color_free (str->color);
  mpui_color_free (str->focused_color);
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
mpui_image_new (char *id, char *file,
                mpui_size_t x, mpui_size_t y, mpui_size_t w, mpui_size_t h)
{
  mpui_image_t *image;

  image = (mpui_image_t *) malloc (sizeof (*image));
  image->id = mpui_strdup (id);
  image->file = mpui_strdup (file);
  image->x = x;
  image->y = y;
  image->w = w;
  image->h = h;
  image->num_planes = 0;
  image->raw.data = NULL;

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
  free (image->raw.data);
  if (image->num_planes > 0)
    free (image->planes[0]);
  free (image);
}

mpui_img_t *
mpui_img_new (mpui_image_t *image, mpui_size_t x, mpui_size_t y,
              mpui_size_t w, mpui_size_t h, mpui_flags_t flags,
              mpui_when_focused_t when_focused)
{
  mpui_img_t *img;

  img = (mpui_img_t *) malloc (sizeof (*img));
  img->element.type = MPUI_IMG;
  img->element.flags = flags;
  img->element.x = x;
  img->element.y = y;
  img->element.w = w;
  img->element.h = h;
  img->element.when_focused = when_focused;
  img->image = image;
  if (!image->raw.data)
    mpui_image_load (image);

  if (img->element.w == 0 && img->element.h == 0)
    {
      img->element.w = image->w;
      img->element.h = image->h;
    }
  else if (image->raw.data)
    {
      if (img->element.w == 0)
        img->element.w = img->element.h*image->raw.w/image->raw.h;
      if (img->element.h == 0)
        img->element.h = img->element.w*image->raw.h/image->raw.w;
    }

  return img;
}

void
mpui_img_load (mpui_t *mpui, mpui_img_t *img)
{
  mpui_image_t *image = img->image;

  if (img->element.w == image->w && img->element.h == image->h)
    {
      if (image->raw.data && image->num_planes <= 0)
        mpui_image_convert (image, &image->raw, mpui->format);
    }
  else
    {
      image = mpui_image_new ("", image->file,
                              img->element.x, img->element.y,
                              img->element.w, img->element.h);
      mpui_images_add (mpui->images[0], image);
      if (img->image->raw.data)
        mpui_image_convert (image, &img->image->raw, mpui->format);
      img->image = image;
    }
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
mpui_font_new (mpui_t *mpui, char *id, char *file, int size,
               mpui_color_t *color, mpui_color_t *focused_color)
{
  mpui_font_t *font;

  font = (mpui_font_t *) malloc (sizeof (*font));
  font->id = mpui_strdup (id);
  font->size = size;
  font->color = color;
  font->focused_color = focused_color;
  font->font_desc = read_font_desc (file, 0.75, 0);
  if (!font->font_desc)
    font->font_desc = read_font_desc_ft (file, size*mpui->width/40,
                                         size*mpui->height/40);
  return font;
}

mpui_font_t *
mpui_font_get (mpui_t *mpui, char *id)
{
  mpui_fonts_t **fonts;
  mpui_font_t **font;

  if (id)
    {
      for (fonts=mpui->fonts; *fonts; fonts++)
        for (font=(*fonts)->fonts; *font; font++)
          if (!strcmp ((*font)->id, id))
            return *font;
    }
  else
    {
      for (fonts=mpui->fonts; *fonts; fonts++)
        if ((*fonts)->deflt)
          return (*fonts)->deflt;
    }
  return NULL;
}

void
mpui_font_free (mpui_font_t *font)
{
  free (font->id);
  mpui_color_free (font->color);
  mpui_color_free (font->focused_color);
  free_font_desc (font->font_desc);
  free (font);
}


mpui_fonts_t *
mpui_fonts_new (void)
{
  mpui_fonts_t *fonts;

  fonts = (mpui_fonts_t *) malloc (sizeof (*fonts));
  fonts->deflt = NULL;
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
mpui_object_new (char *id, mpui_size_t x, mpui_size_t y)
{
  mpui_object_t *object;

  object = (mpui_object_t *) malloc (sizeof (*object));
  object->id = mpui_strdup (id);
  object->x = x;
  object->y = y;
  object->elements = mpui_list_new ();
  object->actions = mpui_list_new ();
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
  mpui_action_t **a = object->actions;

  while (*e)
    mpui_element_free (*e++);
  free (object->elements);
  while (*a)
    mpui_action_free (*a++);
  free (object->actions);
  free (object->id);
  free (object);
}

mpui_obj_t *
mpui_obj_new (mpui_object_t *object, mpui_size_t x, mpui_size_t y,
              mpui_flags_t flags, mpui_when_focused_t when_focused)
{
  mpui_element_t **e;
  mpui_obj_t *obj;

  obj = (mpui_obj_t *) malloc (sizeof (*obj));
  obj->container.element.type = MPUI_OBJ;
  obj->container.element.flags = flags | MPUI_FLAG_CONTAINER;
  obj->container.element.x = x;
  obj->container.element.y = y;
  obj->container.element.when_focused = when_focused;
  obj->container.elements = object->elements;
  obj->container.actions = object->actions;
  obj->object = object;
  mpui_elements_get_size (&obj->container.element, object->elements, NULL);

  if (*object->actions)
    obj->container.element.flags |= MPUI_FLAG_FOCUSABLE;
  else
    for (e=object->elements; *e; e++)
      if ((*e)->when_focused != MPUI_DISPLAY_ALWAYS)
        {
          obj->container.element.flags |= MPUI_FLAG_FOCUSABLE ;
          break;
        }

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
mpui_menu_new (char *id, mpui_orientation_t orientation,
               mpui_size_t x, mpui_size_t y)
{
  mpui_menu_t *menu;

  menu = (mpui_menu_t *) malloc (sizeof (*menu));
  menu->id = mpui_strdup (id);
  menu->orientation = orientation;
  menu->x = x;
  menu->y = y;
  menu->allmenuitem = NULL;
  menu->elements = mpui_list_new ();
  return menu;
}

mpui_menu_t *
mpui_menu_get (mpui_t *mpui, char *id)
{
  mpui_menus_t **menus;
  mpui_menu_t **menu;

  for (menus=mpui->menus; *menus; menus++)
    for (menu=(*menus)->menus; *menu; menu++)
      if (!strcmp ((*menu)->id, id))
        return *menu;
  return NULL;
}

void
mpui_menu_free (mpui_menu_t *menu)
{
  mpui_element_t **e = menu->elements;

  if (menu->allmenuitem)
    mpui_allmenuitem_free (menu->allmenuitem);
  while (*e)
    mpui_element_free (*e++);
  free (menu->elements);
  free (menu);
}

mpui_mnu_t *
mpui_mnu_new (mpui_menu_t *menu, mpui_size_t x, mpui_size_t y,
              mpui_flags_t flags)
{
  mpui_mnu_t *mnu;

  mnu = (mpui_mnu_t *) malloc (sizeof (*mnu));
  mnu->fb.container.element.type = MPUI_MNU;
  mnu->fb.container.element.flags = flags | MPUI_FLAG_CONTAINER;
  mnu->fb.container.element.x = x;
  mnu->fb.container.element.y = y;
  mnu->fb.container.element.w = menu->w;
  mnu->fb.container.element.h = menu->h;
  mnu->fb.container.element.when_focused = MPUI_DISPLAY_ALWAYS;
  mnu->fb.container.elements = menu->elements;
  mnu->fb.container.actions = NULL;
  mnu->fb.orientation = menu->orientation;
  if (mpui_focus_first (&mnu->fb))
    mnu->fb.container.element.flags |= MPUI_FLAG_FOCUS_BOX;
  mnu->menu = menu;
  return mnu;
}

void
mpui_mnu_free (mpui_mnu_t *mnu)
{
  free (mnu);
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
  menuitem->container.element.type = MPUI_MENUITEM;
  menuitem->container.element.flags = MPUI_FLAG_CONTAINER|MPUI_FLAG_FOCUSABLE;
  menuitem->container.element.when_focused = MPUI_DISPLAY_ALWAYS;
  menuitem->container.elements = mpui_list_new ();
  menuitem->container.actions = mpui_list_new ();
  return menuitem;
}

void
mpui_menuitem_free (mpui_menuitem_t *menuitem)
{
  mpui_element_t **e = menuitem->container.elements;
  mpui_action_t **a = menuitem->container.actions;

  while (*e)
    mpui_element_free (*e++);
  free (menuitem->container.elements);
  while (*a)
    mpui_action_free (*a++);
  free (menuitem->container.actions);
  free (menuitem);
}

mpui_allmenuitem_t *
mpui_allmenuitem_new (void)
{
  mpui_allmenuitem_t *allmenuitem;

  allmenuitem = (mpui_allmenuitem_t *) malloc (sizeof (*allmenuitem));
  allmenuitem->elements = mpui_list_new ();
  allmenuitem->actions = mpui_list_new ();
  return allmenuitem;
}

void
mpui_allmenuitem_free (mpui_allmenuitem_t *allmenuitem)
{
  mpui_element_t **e = allmenuitem->elements;
  mpui_action_t **a = allmenuitem->actions;

  while (*e)
    mpui_element_free (*e++);
  free (allmenuitem->elements);
  while (*a)
    mpui_action_free (*a++);
  free (allmenuitem->actions);
  free (allmenuitem);
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


void
mpui_element_free (mpui_element_t *element)
{
  switch (element->type)
    {
    case MPUI_STR:
      mpui_str_free ((mpui_str_t *) element);
      break;
    case MPUI_IMG:
      mpui_img_free ((mpui_img_t *) element);
      break;
    case MPUI_OBJ:
      mpui_obj_free ((mpui_obj_t *) element);
      break;
    case MPUI_MNU:
      mpui_mnu_free ((mpui_mnu_t *) element);
      break;
    case MPUI_MENUITEM:
      mpui_menuitem_free ((mpui_menuitem_t *) element);
      break;
    }
}

void
mpui_elements_get_size (mpui_element_t *element,
                        mpui_element_t **elements, mpui_element_t **elements2)
{
  element->w = -1;
  element->h = -1;

  while (elements)
    {
      while (*elements)
        {
          if (!((*elements)->flags & (MPUI_FLAG_ABSOLUTE|MPUI_FLAG_NOCOORD)))
            {
              if ((*elements)->x + (*elements)->w > element->w)
                element->w = (*elements)->x + (*elements)->w;
              if ((*elements)->y + (*elements)->h > element->h)
                element->h = (*elements)->y + (*elements)->h;
            }
          elements++;
        }
      elements = elements2;
      elements2 = NULL;
    }

  if (element->w == -1 || element->h == -1)
    element->flags |= MPUI_FLAG_NOCOORD;
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
mpui_screen_get (mpui_screens_t *screens, char *id)
{
  mpui_screen_t **screen;

  for (screen = screens->screens; *screen; screen++)
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
  screens->menu = NULL;
  screens->ctrl = NULL;

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
mpui_new (int width, int height, int format)
{
  mpui_t *mpui;

  mpui = (mpui_t *) malloc (sizeof (*mpui));
  mpui->width = width;
  mpui->height = height;
  mpui->format = format;
  mpui->strings = mpui_list_new ();
  mpui->images = mpui_list_new ();
  mpui->fonts = mpui_list_new ();
  mpui->objects = mpui_list_new ();
  mpui->menus = mpui_list_new ();
  mpui->screens = NULL;
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

  if (mpui->screens)
    mpui_screens_free (mpui->screens);
}
