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
mpui_list_empty (void *list)
{
  if (* (void **) list)
    return 0;
  return 1;
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

void
mpui_list_remove_last (void *list)
{
  int n = mpui_list_length (list) - 1;
  void **l = list;
  if (n >= 0)
    l[n] = NULL;
}

void
mpui_list_free (void *list)
{
  free (list);
}


static void
mpui_element_init (mpui_element_t *element,
                   mpui_type_t type, mpui_flags_t flags)
{
  element->type = type;
  element->flags = flags;
  element->x.val = 0;
  element->x.str = NULL;
  element->y.val = 0;
  element->y.str = NULL;
  element->w.val = 0;
  element->w.str = NULL;
  element->h.val = 0;
  element->h.str = NULL;
  element->when_focused = MPUI_DISPLAY_ALWAYS;
}

static void
mpui_element_uninit (mpui_element_t *element)
{
  free (element->x.str);
  free (element->y.str);
  free (element->w.str);
  free (element->h.str);
  element->x.str = NULL;
  element->y.str = NULL;
  element->w.str = NULL;
  element->h.str = NULL;
}

static void
mpui_container_init (mpui_container_t *container,
                     mpui_type_t type, mpui_flags_t flags,
                     mpui_element_t **elements, mpui_action_t **actions)
{
  if (!elements && !actions)
    {
      flags |= MPUI_FLAG_OWNER;
      container->elements = mpui_list_new ();
      container->actions = mpui_list_new ();
    }
  else
    {
      flags &= ~MPUI_FLAG_OWNER;
      container->elements = elements;
      container->actions = actions;
    }
  mpui_element_init ((mpui_element_t *) container,
                     type, flags | MPUI_FLAG_CONTAINER);
}

static void
mpui_container_uninit (mpui_container_t *container)
{
  if (((mpui_element_t *) container)->flags & MPUI_FLAG_OWNER)
    {
      mpui_elements_free (container->elements);
      mpui_actions_free (container->actions);
    }
  mpui_element_uninit ((mpui_element_t *) container);
}

static void
mpui_foxus_box_init (mpui_focus_box_t *focus_box,
                     mpui_type_t type, mpui_flags_t flags,
                     mpui_element_t **elements, mpui_action_t **actions,
                     mpui_orientation_t orientation)
{
  mpui_container_init ((mpui_container_t *) focus_box, type,
                       flags, elements, actions);
  if (mpui_focus_first (focus_box))
    focus_box->container.element.flags |= MPUI_FLAG_FOCUS_BOX;
  focus_box->orientation = orientation;
}

static void
mpui_focus_box_uninit (mpui_focus_box_t *focus_box)
{
  mpui_container_uninit ((mpui_container_t *) focus_box);
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

mpui_color_t *
mpui_color_dup (mpui_color_t *color)
{
  if (!color)
    return NULL;
  return mpui_color_new (color->r, color->g, color->b);
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
  char *dst;

  string = (mpui_string_t *) malloc (sizeof (*string));
  string->id = mpui_strdup (id);
  string->text = dst = (char *) malloc (strlen (str) + 1);
  while (*str)
    {
      if (*str == '\\' && str[1] == 'n')
        {
          *dst++ = '\n';
          str += 2;
        }
      else
        *dst++ = *str++;
    }
  *dst = '\0';
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
  int f, w = 0, wmax = 0, htot = 0;
  char *s;

  for (s=str->string->text; *s; s++)
    {
      if (*s == '\n')
        {
          htot += font->height;
          w = 0;
          continue;
        }

      render_one_glyph (font, *s);
      f = font->font[(int)*s];

      w += font->width[(int)*s] + font->charspace;
      if (w > wmax)
        wmax = w;
    }
  htot += font->height;

  str->element.w.val = wmax;
  str->element.h.val = htot;
}

mpui_str_t *
mpui_str_new (mpui_string_t *string, mpui_coord_t x, mpui_coord_t y,
              mpui_flags_t flags, mpui_font_t *font, int size,
              mpui_color_t *color, mpui_color_t *focused_color,
              mpui_color_t *really_focused_color,
              mpui_when_focused_t when_focused)
{
  mpui_str_t *str;

  str = (mpui_str_t *) malloc (sizeof (*str));
  mpui_element_init ((mpui_element_t *) str, MPUI_STR, flags);
  str->element.x = x;
  str->element.y = y;
  str->element.when_focused = when_focused;
  str->string = string;
  str->font = font;
  str->size = size;
  str->color = color;
  str->focused_color = focused_color;
  str->really_focused_color = really_focused_color;
  mpui_str_get_size (str);
  return str;
}

mpui_str_t *
mpui_str_dup (mpui_str_t *str)
{
  mpui_element_t *elem = (mpui_element_t *) str;
  mpui_str_t *dup;

  dup = mpui_str_new (str->string, elem->x, elem->y, elem->flags,
                      str->font, str->size, mpui_color_dup (str->color),
                      mpui_color_dup (str->focused_color),
                      mpui_color_dup (str->really_focused_color),
                      elem->when_focused);
  mpui_element_coord_dup ((mpui_element_t *) dup);

  return dup;
}

void
mpui_str_free (mpui_str_t *str)
{
  mpui_color_free (str->color);
  mpui_color_free (str->focused_color);
  mpui_color_free (str->really_focused_color);
  mpui_element_uninit ((mpui_element_t *) str);
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
  mpui_image_t **image;

  for (image=mpui->images->images; *image; image++)
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
mpui_img_new (mpui_image_t *image, mpui_coord_t x, mpui_coord_t y,
              mpui_coord_t w, mpui_coord_t h, mpui_flags_t flags,
              mpui_when_focused_t when_focused)
{
  mpui_img_t *img;

  img = (mpui_img_t *) malloc (sizeof (*img));
  mpui_element_init ((mpui_element_t *) img, MPUI_IMG, flags);
  img->element.x = x;
  img->element.y = y;
  img->element.w = w;
  img->element.h = h;
  img->element.when_focused = when_focused;
  img->image = image;
  if (!image->raw.data)
    mpui_image_load (image);

  if (img->element.w.val == 0 && img->element.h.val == 0)
    {
      img->element.w.val = image->w;
      img->element.h.val = image->h;
    }
  else if (image->raw.data)
    {
      if (img->element.w.val == 0)
        img->element.w.val = img->element.h.val*image->raw.w/image->raw.h;
      if (img->element.h.val == 0)
        img->element.h.val = img->element.w.val*image->raw.h/image->raw.w;
    }

  return img;
}

mpui_img_t *
mpui_img_dup (mpui_img_t *img)
{
  mpui_element_t *elem = (mpui_element_t *) img;
  mpui_img_t *dup;

  dup = mpui_img_new (img->image, elem->x, elem->y,
                      elem->w, elem->h, elem->flags,
                      elem->when_focused);
  mpui_element_coord_dup ((mpui_element_t *) dup);

  return dup;
}

void
mpui_img_load (mpui_t *mpui, mpui_img_t *img)
{
  mpui_image_t *image = img->image;

  if (img->element.w.val == image->w && img->element.h.val == image->h)
    {
      if (image->raw.data && image->num_planes <= 0)
        mpui_image_convert (image, &image->raw, mpui->format);
    }
  else
    {
      image = mpui_image_new ("", image->file,
                              img->element.x.val, img->element.y.val,
                              img->element.w.val, img->element.h.val);
      mpui_images_add (mpui->images, image);
      if (img->image->raw.data)
        mpui_image_convert (image, &img->image->raw, mpui->format);
      img->image = image;
    }
}

void
mpui_img_free (mpui_img_t *img)
{
  mpui_element_uninit ((mpui_element_t *) img);
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
               mpui_color_t *color, mpui_color_t *focused_color,
               mpui_color_t *really_focused_color)
{
  mpui_font_t *font;

  font = (mpui_font_t *) malloc (sizeof (*font));
  font->id = mpui_strdup (id);
  font->size = size;
  font->color = color;
  font->focused_color = focused_color;
  font->really_focused_color = really_focused_color;
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
  mpui_color_free (font->really_focused_color);
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
  object->need_dup = 0;
  object->x = x;
  object->y = y;
  object->elements = mpui_list_new ();
  object->actions = mpui_list_new ();
  return object;
}

mpui_object_t *
mpui_object_get (mpui_t *mpui, char *id)
{
  mpui_object_t **object;

  for (object=mpui->objects->objects; *object; object++)
    if (!strcmp ((*object)->id, id))
      return *object;
  return NULL;
}

mpui_object_t *
mpui_object_dup (mpui_object_t *object)
{
  mpui_element_t **e;
  mpui_object_t *dup;

  dup = mpui_object_new ("", object->x, object->y);
  for (e=object->elements; *e; e++)
    mpui_add_element (dup, mpui_element_dup (*e));

  return dup;
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
mpui_obj_new (mpui_object_t *object, mpui_coord_t x, mpui_coord_t y,
              mpui_flags_t flags, mpui_when_focused_t when_focused)
{
  mpui_element_t **e;
  mpui_obj_t *obj;

  if (*object->actions)
    flags |= MPUI_FLAG_FOCUSABLE;
  else
    for (e=object->elements; *e; e++)
      if ((*e)->when_focused != MPUI_DISPLAY_ALWAYS)
        {
          flags |= MPUI_FLAG_FOCUSABLE ;
          break;
        }

  obj = (mpui_obj_t *) malloc (sizeof (*obj));
  mpui_container_init ((mpui_container_t *) obj, MPUI_OBJ, flags,
                       object->elements, object->actions);
  obj->container.element.x = x;
  obj->container.element.y = y;
  obj->container.element.when_focused = when_focused;
  obj->object = object;
  mpui_elements_get_size ((mpui_element_t *) obj, object->elements, NULL);

  if (obj->container.element.flags & MPUI_FLAG_NOCOORD)
    object->need_dup = 1;

  return obj;
}

mpui_obj_t *
mpui_obj_dup (mpui_obj_t *obj)
{
  mpui_element_t *elem = (mpui_element_t *) obj;
  mpui_obj_t *dup;

  dup = mpui_obj_new (obj->object, elem->x, elem->y, elem->flags,
                      elem->when_focused);

  return dup;
}

void
mpui_obj_free (mpui_obj_t *obj)
{
  mpui_container_uninit ((mpui_container_t *) obj);
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
  mpui_menu_t **menu;

  for (menu=mpui->menus->menus; *menu; menu++)
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
mpui_mnu_new (mpui_menu_t *menu, mpui_coord_t x, mpui_coord_t y,
              mpui_flags_t flags)
{
  mpui_mnu_t *mnu;

  mnu = (mpui_mnu_t *) malloc (sizeof (*mnu));
  mpui_foxus_box_init ((mpui_focus_box_t *) mnu, MPUI_MNU, flags,
                       menu->elements, NULL, menu->orientation);
  mnu->fb.container.element.x = x;
  mnu->fb.container.element.y = y;
  mnu->fb.container.element.w.val = menu->w;
  mnu->fb.container.element.h.val = menu->h;
  mnu->menu = menu;
  return mnu;
}

void
mpui_mnu_free (mpui_mnu_t *mnu)
{
  mpui_focus_box_uninit ((mpui_focus_box_t *) mnu);
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
  mpui_container_init ((mpui_container_t *) menuitem, MPUI_MENUITEM,
                       MPUI_FLAG_FOCUSABLE, NULL, NULL);
  return menuitem;
}

void
mpui_menuitem_free (mpui_menuitem_t *menuitem)
{
  mpui_container_uninit ((mpui_container_t *) menuitem);
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
mpui_actions_free (mpui_action_t **actions)
{
  mpui_action_t **a = actions;

  while (*a)
    mpui_action_free (*a++);
  mpui_list_free (actions);
}


mpui_element_t *
mpui_element_dup (mpui_element_t *element)
{
  switch (element->type)
    {
    case MPUI_STR:
      return (mpui_element_t *) mpui_str_dup ((mpui_str_t *) element);
    case MPUI_IMG:
      return (mpui_element_t *) mpui_img_dup ((mpui_img_t *) element);
    case MPUI_OBJ:
      return (mpui_element_t *) mpui_obj_dup ((mpui_obj_t *) element);
    default:
      return NULL;
    }
}

void
mpui_element_coord_dup (mpui_element_t *element)
{
  element->x.str = mpui_strdup (element->x.str);
  element->y.str = mpui_strdup (element->y.str);
  element->w.str = mpui_strdup (element->w.str);
  element->h.str = mpui_strdup (element->h.str);
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
    case MPUI_POPUP:
      mpui_popup_free ((mpui_popup_t *) element);
      break;
    }
}

void
mpui_elements_get_size (mpui_element_t *element,
                        mpui_element_t **elements, mpui_element_t **elements2)
{
  element->w.val = 0;
  element->h.val = 0;

  while (elements)
    {
      while (*elements)
        {
          if (!((*elements)->flags & (MPUI_FLAG_ABSOLUTE|MPUI_FLAG_NOCOORD)))
            {
              if ((*elements)->x.val + (*elements)->w.val > element->w.val)
                element->w.val = (*elements)->x.val + (*elements)->w.val;
              if ((*elements)->y.val + (*elements)->h.val > element->h.val)
                element->h.val = (*elements)->y.val + (*elements)->h.val;
            }
          elements++;
        }
      elements = elements2;
      elements2 = NULL;
    }

  if (element->w.val == 0 || element->h.val == 0)
    element->flags |= MPUI_FLAG_NOCOORD;
}

void
mpui_elements_free (mpui_element_t **elements)
{
  mpui_element_t **e = elements;

  while (*e)
    mpui_element_free (*e++);
  mpui_list_free (elements);
}


mpui_popup_t *
mpui_popup_new (char *id, mpui_coord_t x, mpui_coord_t y)
{
  mpui_popup_t *popup;

  popup = (mpui_popup_t *) malloc (sizeof (*popup));
  mpui_container_init ((mpui_container_t *) popup, MPUI_POPUP, 0, 
                       NULL, NULL);
  popup->container.element.x = x;
  popup->container.element.y = y;
  popup->id = mpui_strdup (id);
  return popup;
}

mpui_popup_t *
mpui_popup_get (mpui_popups_t *popups, char *id)
{
  mpui_popup_t **popup;

  for (popup=popups->popups; *popup; popup++)
    if (!strcmp ((*popup)->id, id))
      return *popup;
  return NULL;
}

void
mpui_popup_free (mpui_popup_t *popup)
{
  mpui_container_uninit ((mpui_container_t *) popup);
  free (popup);
}


mpui_popups_t *
mpui_popups_new (void)
{
  mpui_popups_t *popups;

  popups = (mpui_popups_t *) malloc (sizeof (*popups));
  popups->popups = mpui_list_new ();
  return popups;
}

void
mpui_popups_free (mpui_popups_t *popups)
{
  mpui_popup_t **p = popups->popups;

  while (*p)
    mpui_popup_free (*p++);
  free (popups->popups);
  free (popups);
}


mpui_screen_t *
mpui_screen_new (char *id)
{
  mpui_screen_t *screen;

  screen = (mpui_screen_t *) malloc (sizeof (*screen));
  screen->id = mpui_strdup (id);
  screen->elements = mpui_list_new ();
  screen->popup_stack = mpui_list_new ();
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
  free (screen->popup_stack);
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
  mpui->fonts = mpui_list_new ();
  mpui->images = mpui_images_new ();
  mpui->objects = mpui_objects_new ();
  mpui->menus = mpui_menus_new ();
  mpui->popups = mpui_popups_new ();
  mpui->screens = NULL;
  mpui->current_screen = NULL;
  mpui->previous_screen = NULL;
  return mpui;
}

void
mpui_free (mpui_t *mpui)
{
  mpui_strings_t **strings = mpui->strings;
  mpui_fonts_t **fonts = mpui->fonts;

  while (*strings)
    mpui_strings_free (*strings++);
  free (mpui->strings);

  while (*fonts)
    mpui_fonts_free (*fonts++);
  free (mpui->fonts);

  if (mpui->images)
    mpui_images_free (mpui->images);

  if (mpui->objects)
    mpui_objects_free (mpui->objects);

  if (mpui->menus)
    mpui_menus_free (mpui->menus);

  if (mpui->popups)
    mpui_popups_free (mpui->popups);

  if (mpui->screens)
    mpui_screens_free (mpui->screens);
}
