/*
 *  mpui_struct.c: libmpui structures storage.
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "mpui_struct.h"
#include "mpui_focus.h"
#include "mpui_image.h"
#include "mpui_slideshow.h"


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
mpui_list_free (void *list, mpui_list_free_func_t func)
{
  if (func)
    {
      void **elm = list;
      while (*elm)
        func (*elm++);
    }
  free (list);
}


static void
mpui_element_init (mpui_element_t *element, char *id,
                   mpui_type_t type, mpui_flags_t flags)
{
  element->id = mpui_strdup (id);
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
  free (element->id);
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
mpui_container_init (mpui_container_t *container, char *id,
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
  mpui_element_init ((mpui_element_t *) container, id,
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
mpui_foxus_box_init (mpui_focus_box_t *focus_box, char *id,
                     mpui_type_t type, mpui_flags_t flags,
                     mpui_element_t **elements, mpui_action_t **actions,
                     mpui_orientation_t orientation,
                     mpui_orientation_t scrolling)
{
  mpui_container_init ((mpui_container_t *) focus_box, id, type,
                       flags, elements, actions);
  if (mpui_focus_first (focus_box))
    focus_box->container.element.flags |= MPUI_FLAG_FOCUS_BOX;
  focus_box->orientation = orientation;
  focus_box->scrolling = scrolling;
  focus_box->xoffset = 0;
  focus_box->yoffset = 0;
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
mpui_string_get (mpui_t *mpui, char *id)
{
  mpui_string_t **string;

  if (mpui->strings)
    for (string=mpui->strings->strings; *string; string++)
      if (!strcmp ((*string)->id, id))
        return *string;

  return NULL;
}

static inline unsigned int
mpui_string_get_next_char_real (const int sanity, unsigned char **txt,
                                size_t *len, const mpui_encoding_t encoding)
{
  unsigned int c;

  if (sanity)
    {
      if (!*len)
        return 0;
      (*len)--;
    }
  c = *(*txt)++;

  switch (encoding)
    {
    case MPUI_ENCODING_ISO_8859_1:
      break;
    case MPUI_ENCODING_UTF8:
    {
      size_t ulen = 0;
      if ((c & 0x80) == 0x00)
        break;

      if ((c & 0xE0) == 0xC0)       { ulen = 1; c &= 0x1F; } /* c = 110xxxxx */
      else if ((c & 0xF0) == 0xE0)  { ulen = 2; c &= 0x0F; } /* c = 1110xxxx */
      else if ((c & 0xF8) == 0xF0)  { ulen = 3; c &= 0x07; } /* c = 11110xxx */
      else if ((c & 0xFC) == 0xF8)  { ulen = 4; c &= 0x02; } /* c = 111110xx */
      else if ((c & 0xFE) == 0xFC)  { ulen = 5; c &= 0x01; } /* c = 1111110x */
      else if (sanity)              { return 0; }

      if (sanity)
        {
          if (*len < ulen)
            return 0;
          (*len) -= ulen;
        }

      while (ulen--)
        c = (c << 6) | (*(*txt)++ & 0x3F); /* **txt = 10xxxxxx */
      break;
    }
    case MPUI_ENCODING_UTF16:
      if (sanity)
        {
          if (*len == 0)
            return 0;
          (*len)--;
        }
      c = (c<<8) + *(*txt)++;
      if (c >= 0xD800 && c <= 0xDFFF)  
        {
          if (sanity)
            {
              if (c > 0xDBFF || /* c = 110110yyyyyyyyyy  */
                  *len < 2 ||   /* there's another 16bit */
                  **txt < 0xDC || 0xDF < **txt) /* **txt = 110111xx */
                return 0;
              (*len) -= 2;
            }
          c = (c & 0x3FF) << 10;       /* c = yyyyyyyyyy0000000000 */
          c |= (*(*txt)++ & 0x3) << 8; /* c = yyyyyyyyyyxx00000000 */
          c |= (*(*txt)++);            /* c = yyyyyyyyyyxxxxxxxxxx */
          c += 0x10000;                /* 0x10000 <= c <= 0x10FFFF */
        }
      break;
    }
  return c;
}

unsigned int
mpui_string_get_next_char (unsigned char **txt, mpui_encoding_t encoding)
{
  return mpui_string_get_next_char_real(0, txt, NULL, encoding);
}

static unsigned int
mpui_string_get_next_char_sanity (unsigned char **txt, size_t *len,
                                  mpui_encoding_t encoding)
{
  return mpui_string_get_next_char_real(1, txt, len, encoding);
}

static void
mpui_string_put_next_char (unsigned char **txt, size_t *len,
                           unsigned int c, mpui_encoding_t encoding)
{
  switch (encoding)
    {
    case MPUI_ENCODING_ISO_8859_1:
      if (c < 0x100) /* c is 8bit = xxxxxxxx */ 
        {
          if (!*len)
	    break;
          *(*txt)++ = c; /* encode xxxxxxxx */
          (*len)--;
        }
      break;
    case MPUI_ENCODING_UTF8:
    {
      unsigned char u[6];
      size_t ulen, i;

      if (c < 0x80)            { ulen = 1; u[0] = 0x00; } /* 0xxxxxxx  7bit */
      else if (c < 0x800)      { ulen = 2; u[0] = 0xC0; } /* 110xxxxx 11bit */
      else if (c < 0x10000)    { ulen = 3; u[0] = 0xE0; } /* 1110xxxx 16bit */
      else if (c < 0x200000)   { ulen = 4; u[0] = 0xF0; } /* 11110xxx 21bit */
      else if (c < 0x4000000)  { ulen = 5; u[0] = 0xF8; } /* 111110xx 26bit */
      else if (c < 0x80000000) { ulen = 6; u[0] = 0xFC; } /* 1111110x 31bit */
      else                     { break; }

      if (*len < ulen)
        break;
      (*len) -= ulen;

      i = ulen;
      while (--i)
        {
	  u[i] = (c & 0x3F) | 0x80; /* encode 10xxxxxx */
	  c >>= 6;
        }
      u[0] |= c;

      for (i = 0; i < ulen; i++)
        *(*txt)++ = u[i];
      break;
    }
    case MPUI_ENCODING_UTF16:
      if (c < 0x10000) /* c is 16bit = yyyyyyyyxxxxxxxx */
        {
          if (*len < 2)
            break;
          *(*txt)++ = c >> 8;   /* encode yyyyyyyy */
          *(*txt)++ = c & 0xFF; /* encode xxxxxxxx */
          (*len) -= 2;
        }
      else if (c <= 0x10FFFF)
        {
          if (*len < 4)
            break;
          c -= 0x10000; /* c is 20bit = yyyyyyyyyyxxxxxxxxxx */
          *(*txt)++ = (c >> 18) | 0xD8;        /* encode 110110yy */
          *(*txt)++ = (c >> 10) & 0xFF;        /* encode yyyyyyyy */
          *(*txt)++ = ((c >> 8) & 0x3) | 0xDC; /* encode 110111xx */
          *(*txt)++ = c & 0xFF;                /* encode xxxxxxxx */
          (*len) -= 4;
        }
      break;
    }
}

static size_t
mpui_string_strlen16bit (const unsigned char *str)
{
  const unsigned char *s;
  for (s = str++; *s++ || *s; s++)
    ;
  return (s - str);
}

mpui_string_t *
mpui_string_new (char *id, unsigned char *str, mpui_encoding_t encoding)
{
  mpui_string_t *string;
  unsigned char *dst;
  size_t dlen, len;
  unsigned int c;

  /* find length in bytes (including the terminating '\0' character) */
  switch (encoding)
    {
    case MPUI_ENCODING_ISO_8859_1:
    case MPUI_ENCODING_UTF8:
      len = strlen(str) + 1;
      break;
    case MPUI_ENCODING_UTF16:
      len = mpui_string_strlen16bit(str) + 2;
      break;
    }

  string = (mpui_string_t *) malloc (sizeof (*string));
  string->id = mpui_strdup (id);
  dlen = len;
  string->text = dst = (char *) malloc (dlen);
  string->encoding = encoding;

  do {
    c = mpui_string_get_next_char_sanity (&str, &len, encoding);
    while (c == '\\')
      {
        c = mpui_string_get_next_char_sanity (&str, &len, encoding);
        if (c == 'n')
          {
            mpui_string_put_next_char (&dst, &dlen, '\n', encoding);
            c = mpui_string_get_next_char_sanity (&str, &len, encoding);
            break;
          }
        mpui_string_put_next_char (&dst, &dlen, '\\', encoding);
      }
    mpui_string_put_next_char (&dst, &dlen, c, encoding);
  } while (c);

  return string;
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
  unsigned int c;
  int w = 0, wmax = 0, htot = 0;
  unsigned char *txt = str->string->text;

  while ((c = mpui_string_get_next_char (&txt, str->string->encoding)))
    {
      if (c == '\n')
        {
          htot += font->height;
          w = 0;
          continue;
        }

      render_one_glyph (font, c);
      w += font->width[c] + font->charspace;
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
  mpui_element_init ((mpui_element_t *) str, string->id, MPUI_STR, flags);
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
mpui_strings_new (char *fonts, char *lang)
{
  mpui_strings_t *strings;

  strings = (mpui_strings_t *) malloc (sizeof (*strings));
  strings->fonts = mpui_strdup (fonts);
  strings->lang = mpui_strdup (lang);
  strings->strings = mpui_list_new ();
  return strings;
}

void
mpui_strings_free (mpui_strings_t *strings)
{
  free (strings->fonts);
  free (strings->lang);
  mpui_list_free (strings->strings, (mpui_list_free_func_t) mpui_string_free);
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
mpui_image_dup (mpui_image_t *image, mpui_size_t x, mpui_size_t y,
                mpui_size_t w, mpui_size_t h)
{
  mpui_image_t *dup;

  dup = mpui_image_new ("", image->file, x, y, w, h);
  memcpy (&dup->raw, &image->raw, sizeof (dup->raw));
  dup->dup = 1;
  return dup;
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
  if (!image->dup)
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
  mpui_element_init ((mpui_element_t *) img, image->id, MPUI_IMG, flags);
  img->element.x = x;
  img->element.y = y;
  img->element.w = w;
  img->element.h = h;
  img->element.when_focused = when_focused;
  img->image = image;
  img->dup = 0;
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
      image = mpui_image_dup (image, img->element.x.val, img->element.y.val,
                              img->element.w.val, img->element.h.val);
      if (image->raw.data)
        mpui_image_convert (image, &image->raw, mpui->format);
      if (img->dup)
        mpui_image_free (img->image);
      img->image = image;
      img->dup = 1;
    }
}

void
mpui_img_free (mpui_img_t *img)
{
  if (img->dup)
    {
      img->image->raw.data = NULL;
      mpui_image_free (img->image);
    }
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
  mpui_list_free (images->images, (mpui_list_free_func_t) mpui_image_free);
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
  mpui_font_t **font;

  if (id)
    {
      if (mpui->fonts)
        for (font=mpui->fonts->fonts; *font; font++)
          if (!strcmp ((*font)->id, id))
            return *font;
    }
  else
    {
      if (mpui->fonts && mpui->fonts->deflt)
        return mpui->fonts->deflt;
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
  mpui_list_free (fonts->fonts, (mpui_list_free_func_t) mpui_font_free);
  free (fonts);
}


mpui_filetype_t *
mpui_filetype_new (mpui_match_t match)
{
  mpui_filetype_t *filetype;

  filetype = (mpui_filetype_t *) malloc (sizeof (*filetype));
  filetype->match = match;
  filetype->icon = NULL;
  filetype->actions = mpui_list_new ();
  filetype->exts = mpui_list_new ();
  filetype->dup = 0;
  return filetype;
}

mpui_filetype_t *
mpui_filetype_dup (mpui_filetype_t *filetype,
                   mpui_size_t icon_w, mpui_size_t icon_h)
{
  mpui_filetype_t *dup;

  dup = (mpui_filetype_t *) malloc (sizeof (*dup));
  dup->match = filetype->match;
  dup->icon = mpui_image_dup (filetype->icon, 0, 0, icon_w, icon_h);
  dup->actions = filetype->actions;
  dup->exts = filetype->exts;
  dup->dup = 1;
  return dup;
}

void
mpui_filetype_free (mpui_filetype_t *filetype)
{
  if (!filetype->dup)
    {
      mpui_list_free (filetype->actions, (mpui_list_free_func_t) mpui_action_free);
      mpui_list_free (filetype->exts, (mpui_list_free_func_t) free);
    }
  free (filetype);
}

mpui_filetypes_t *
mpui_filetypes_new (char *id)
{
  mpui_filetypes_t *filetypes;

  filetypes = (mpui_filetypes_t *) malloc (sizeof (*filetypes));
  filetypes->id = mpui_strdup (id);
  filetypes->filetypes = mpui_list_new ();
  return filetypes;
}

mpui_filetypes_t *
mpui_filetypes_get (mpui_t *mpui, char *id)
{
  mpui_filetypes_t **filetypes;

  for (filetypes=mpui->filetypes; *filetypes; filetypes++)
    if (!strcmp ((*filetypes)->id, id))
      return *filetypes;
  return NULL;
}

mpui_filetypes_t *
mpui_filetypes_dup (mpui_filetypes_t *filetypes,
                    mpui_size_t icon_w, mpui_size_t icon_h)
{
  mpui_filetype_t **filetype;
  mpui_filetypes_t *dup;

  dup = mpui_filetypes_new ("");
  for (filetype=filetypes->filetypes; *filetype; filetype++)
    mpui_filetypes_add (dup, mpui_filetype_dup (*filetype, icon_w, icon_h));
  return dup;
}

void
mpui_filetypes_free (mpui_filetypes_t *filetypes)
{
  mpui_list_free (filetypes->filetypes, (mpui_list_free_func_t) mpui_filetype_free);
  free (filetypes->id);
  free (filetypes);
}


mpui_object_t *
mpui_object_new (char *id, mpui_size_t x, mpui_size_t y)
{
  mpui_object_t *object;

  object = (mpui_object_t *) malloc (sizeof (*object));
  object->id = mpui_strdup (id);
  object->need_dup = 0;
  object->dup = 0;
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
    mpui_object_elements_add (dup, mpui_element_dup (*e));
  dup->dup = 1;

  return dup;
}

void
mpui_object_free (mpui_object_t *object)
{
  mpui_list_free (object->elements, (mpui_list_free_func_t) mpui_element_free);
  mpui_list_free (object->actions, (mpui_list_free_func_t) mpui_action_free);
  free (object->id);
  free (object);
}

mpui_obj_t *
mpui_obj_new (mpui_object_t *object, mpui_coord_t x, mpui_coord_t y,
              mpui_flags_t flags, mpui_when_focused_t when_focused)
{
  mpui_element_t **e;
  mpui_obj_t *obj;

  if (object->need_dup)
    object = mpui_object_dup (object);

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
  mpui_container_init ((mpui_container_t *) obj, object->id, MPUI_OBJ, flags,
                       object->elements, object->actions);
  obj->container.element.x = x;
  obj->container.element.y = y;
  obj->container.element.when_focused = when_focused;
  obj->object = object;
  mpui_elements_get_size ((mpui_element_t *) obj, object->elements);

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
  if (obj->object->dup)
    mpui_object_free (obj->object);
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
  mpui_list_free (objects->objects, (mpui_list_free_func_t) mpui_object_free);
  free (objects);
}


mpui_menu_t *
mpui_menu_new (char *id, mpui_orientation_t orientation,
               mpui_size_t x, mpui_size_t y, mpui_font_t *font)
{
  mpui_menu_t *menu;

  menu = (mpui_menu_t *) malloc (sizeof (*menu));
  menu->id = mpui_strdup (id);
  menu->is_browser = 0;
  menu->is_playlist = 0;
  menu->need_refresh = 0;
  menu->orientation = orientation;
  menu->scrolling = 0;
  menu->x = x;
  menu->y = y;
  menu->font = font;
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
  mpui_list_free (menu->elements, (mpui_list_free_func_t) mpui_element_free);
  free (menu);
}

mpui_mnu_t *
mpui_mnu_new (mpui_menu_t *menu, mpui_coord_t x, mpui_coord_t y,
              mpui_flags_t flags)
{
  mpui_mnu_t *mnu;

  mnu = (mpui_mnu_t *) malloc (sizeof (*mnu));
  mpui_foxus_box_init ((mpui_focus_box_t *) mnu, menu->id, MPUI_MNU, flags,
                       menu->elements, NULL,
                       menu->orientation, menu->scrolling);
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
  mpui_list_free (menus->menus, (mpui_list_free_func_t) mpui_menu_free);
  free (menus);
}


mpui_menuitem_t *
mpui_menuitem_new (void)
{
  mpui_menuitem_t *menuitem;

  menuitem = (mpui_menuitem_t *) malloc (sizeof (*menuitem));
  mpui_container_init ((mpui_container_t *) menuitem, NULL, MPUI_MENUITEM,
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
mpui_allmenuitem_new (mpui_menu_t *menu)
{
  mpui_allmenuitem_t *allmenuitem;

  allmenuitem = (mpui_allmenuitem_t *) malloc (sizeof (*allmenuitem));
  mpui_container_init ((mpui_container_t *) allmenuitem, NULL,
                       MPUI_ALLMENUITEM, MPUI_FLAG_DYNAMIC, NULL, NULL);
  allmenuitem->menu = menu;
  return allmenuitem;
}

void
mpui_allmenuitem_free (mpui_allmenuitem_t *allmenuitem)
{
  mpui_container_uninit ((mpui_container_t *) allmenuitem);
  free (allmenuitem);
}

mpui_action_t *
mpui_action_new (char *cmd, mpui_action_when_t when)
{
  mpui_action_t *action;

  action = (mpui_action_t *) malloc (sizeof (*action));
  action->cmd = mpui_strdup (cmd);
  action->when = when;
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
  mpui_list_free (actions, (mpui_list_free_func_t) mpui_action_free);
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
    case MPUI_ANY:
      break;
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
    case MPUI_ALLMENUITEM:
      mpui_allmenuitem_free ((mpui_allmenuitem_t *) element);
      break;
    case MPUI_POPUP:
      mpui_popup_free ((mpui_popup_t *) element);
      break;
    case MPUI_INF:
      mpui_inf_free ((mpui_inf_t *) element);
      break;
    case MPUI_SLIDESHOW:
      mpui_slideshow_free ((mpui_slideshow_t *) element);
      break;
    }
}

void
mpui_elements_get_size (mpui_element_t *element, mpui_element_t **elements)
{
  element->w.val = 0;
  element->h.val = 0;

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

  if (element->w.val == 0 || element->h.val == 0)
    element->flags |= MPUI_FLAG_NOCOORD;
}

void
mpui_elements_free (mpui_element_t **elements)
{
  mpui_list_free (elements, (mpui_list_free_func_t) mpui_element_free);
}


mpui_browser_t *
mpui_browser_new (char *id, mpui_font_t *font, mpui_orientation_t orientation,
                  mpui_orientation_t scrolling, mpui_alignment_t align,
                  mpui_size_t x, mpui_size_t y, mpui_size_t w, mpui_size_t h,
                  mpui_size_t icon_w, mpui_size_t icon_h,
                  mpui_size_t item_w, mpui_size_t spacing,
                  mpui_object_t *border, mpui_object_t *item_border,
                  mpui_filetypes_t *filter)
{
  mpui_browser_t *browser;
  mpui_coord_t tx, ty;

  browser = (mpui_browser_t *) malloc (sizeof (*browser));
  browser->menu.id = mpui_strdup (id);
  browser->menu.is_browser = 1;
  browser->menu.is_playlist = 0;
  browser->menu.need_refresh = 0;
  browser->menu.orientation = orientation;
  browser->menu.scrolling = scrolling;
  browser->menu.x = x;
  browser->menu.y = y;
  browser->menu.w = w;
  browser->menu.h = h;
  browser->menu.font = font;
  browser->menu.elements = mpui_list_new ();
  browser->icon_w = icon_w;
  browser->icon_h = icon_h;
  browser->item_w = item_w;
  browser->spacing = spacing;
  browser->align = align;
  browser->filter = filter;
  if (border)
    {
      tx.val = border->x;
      tx.str = NULL;
      ty.val = border->y;
      ty.str = NULL;
      browser->border = mpui_obj_new (border, tx, ty, 0, MPUI_DISPLAY_ALWAYS);
    }
  else
    browser->border = NULL;
  if (item_border)
    {
      mpui_obj_t *obj;
      tx.val = item_border->x;
      tx.str = NULL;
      ty.val = item_border->y;
      ty.str = NULL;
      obj = mpui_obj_new (item_border, tx, ty, 0, MPUI_DISPLAY_ALWAYS);
      browser->item_border = mpui_allmenuitem_new ((mpui_menu_t *) browser);
      mpui_container_elements_add ((mpui_container_t *) browser->item_border,
                                   (mpui_element_t *) obj);
    }
  else
    browser->item_border = NULL;
  return browser;
}

void
mpui_browser_free (mpui_browser_t *browser)
{
  mpui_filetypes_free (browser->filter);
  mpui_menu_free (&browser->menu);
}

mpui_playlist_t *
mpui_playlist_new (char *id, mpui_font_t *font, mpui_orientation_t orientation,
                   mpui_orientation_t scrolling, mpui_alignment_t align,
                   mpui_size_t x, mpui_size_t y, mpui_size_t w, mpui_size_t h,
                   mpui_size_t item_w, mpui_size_t spacing,
                   mpui_object_t *border, mpui_object_t *item_border)
{
  mpui_playlist_t *playlist;
  mpui_coord_t tx, ty;

  playlist = (mpui_playlist_t *) malloc (sizeof (*playlist));
  playlist->menu.id = mpui_strdup (id);
  playlist->menu.is_browser = 0;
  playlist->menu.is_playlist = 1;
  playlist->menu.need_refresh = 0;
  playlist->menu.orientation = orientation;
  playlist->menu.scrolling = scrolling;
  playlist->menu.x = x;
  playlist->menu.y = y;
  playlist->menu.w = w;
  playlist->menu.h = h;
  playlist->menu.font = font;
  playlist->menu.elements = mpui_list_new ();
  playlist->item_w = item_w;
  playlist->spacing = spacing;
  playlist->align = align;

  if (border)
    {
      tx.val = border->x;
      tx.str = NULL;
      ty.val = border->y;
      ty.str = NULL;
      playlist->border = mpui_obj_new (border, tx, ty, 0, MPUI_DISPLAY_ALWAYS);
    }
  else
    playlist->border = NULL;

  if (item_border)
    {
      mpui_obj_t *obj;
      tx.val = item_border->x;
      tx.str = NULL;
      ty.val = item_border->y;
      ty.str = NULL;
      obj = mpui_obj_new (item_border, tx, ty, 0, MPUI_DISPLAY_ALWAYS);
      playlist->item_border = mpui_allmenuitem_new ((mpui_menu_t *) playlist);
      mpui_container_elements_add ((mpui_container_t *) playlist->item_border,
                                   (mpui_element_t *) obj);
    }
  else
    playlist->item_border = NULL;

  return playlist;
}

void
mpui_playlist_free (mpui_playlist_t *playlist)
{
  mpui_menu_free (&playlist->menu);
}

mpui_tag_t *
mpui_tag_new (char *id, char *caption, char *type,
              mpui_coord_t x, mpui_coord_t y)
{
  mpui_tag_t *tag;

  tag = (mpui_tag_t *) malloc (sizeof (*tag));
  tag->id = mpui_strdup (id);
  tag->caption = mpui_strdup (caption);
  tag->type = mpui_strdup (type);
  tag->x = x;
  tag->y = y;

  return tag;
}

void
mpui_tag_free (mpui_tag_t *tag)
{
  free (tag->id);
  free (tag->caption);
  free (tag->type);
  free (tag);
}

mpui_pic_t *
mpui_pic_new (char *id, mpui_coord_t x, mpui_coord_t y,
              mpui_coord_t w, mpui_coord_t h, mpui_filetypes_t *filter)
{
  mpui_pic_t *pic;

  pic = (mpui_pic_t *) malloc (sizeof (*pic));
  pic->id = mpui_strdup (id);
  pic->x = x;
  pic->y = y;
  pic->w = w;
  pic->h = h;
  pic->filter = filter;

  return pic;
}

void
mpui_pic_free (mpui_pic_t *pic)
{
  free (pic->id);
  free (pic);
}

mpui_info_t *
mpui_info_new (char *id, mpui_font_t *font, mpui_coord_t x, mpui_coord_t y,
               mpui_coord_t w, mpui_coord_t h)
{
  mpui_info_t *info;
    
  info = (mpui_info_t *) malloc (sizeof (*info));
  info->id = mpui_strdup (id);
  info->font = font;
  info->x = x;
  info->y = y;
  info->w = w;
  info->h = h;
  info->tags = mpui_list_new ();
  info->pics = mpui_list_new ();

  return info;
}

mpui_info_t *
mpui_info_get (mpui_t *mpui, char *id)
{
  mpui_info_t **info;

  for (info = mpui->infos->infos; *info; info++)
    if (!strcmp ((*info)->id, id))
      return *info;
  return NULL;
}

void
mpui_info_free (mpui_info_t *info)
{
  mpui_list_free (info->tags, (mpui_list_free_func_t) mpui_tag_free);
  mpui_list_free (info->pics, (mpui_list_free_func_t) mpui_pic_free);
  free (info->id);
  free (info);
}

mpui_inf_t *
mpui_inf_new (mpui_info_t *info, mpui_coord_t x, mpui_coord_t y,
              mpui_when_focused_t when_focused, unsigned int timer)
{
  mpui_inf_t *inf;

  inf = (mpui_inf_t *) malloc (sizeof (*inf));
  mpui_container_init ((mpui_container_t *) inf, info->id, MPUI_INF,
                       0, NULL, NULL);
  inf->container.element.x = x;
  inf->container.element.y = y;
  inf->container.element.when_focused = when_focused;
  inf->info = info;
  inf->filename_id = inf->last_filename_id = 0;
  inf->filename[sizeof (inf->filename) - 1] = '\0';
  inf->timer = timer;

  return inf;
}

void
mpui_inf_free (mpui_inf_t *inf)
{
  mpui_container_uninit ((mpui_container_t *) inf);
  free (inf);
}

mpui_infos_t *
mpui_infos_new (void)
{
  mpui_infos_t *infos;

  infos = (mpui_infos_t *) malloc (sizeof (*infos));
  infos->infos = mpui_list_new ();
  return infos;
}

void
mpui_infos_free (mpui_infos_t *infos)
{
  mpui_list_free (infos->infos, (mpui_list_free_func_t) mpui_info_free);
  free (infos);
}

mpui_slideshow_t *
mpui_slideshow_new (char *id, mpui_coord_t x, mpui_coord_t y,
                    mpui_coord_t w, mpui_coord_t h, char *path,
                    mpui_filetypes_t *filter, char *mode,
                    int timer, mpui_coord_t name_x, mpui_coord_t name_y,
                    mpui_font_t *name_font, mpui_object_t *border)
{
  mpui_slideshow_t *slideshow;

  slideshow = (mpui_slideshow_t *) malloc (sizeof (*slideshow));
  mpui_container_init ((mpui_container_t *) slideshow, id, MPUI_SLIDESHOW,
                       0, NULL, NULL);
  slideshow->container.element.x = x;
  slideshow->container.element.y = y;
  slideshow->container.element.w = w;
  slideshow->container.element.h = h;
  slideshow->path_id = slideshow->last_path_id = 0;
  slideshow->path[sizeof (slideshow->path) - 1] = '\0';
  slideshow->name = NULL;
  mpui_slideshow_path (slideshow, path);
  slideshow->filter = filter;
  slideshow->mode = -1;
  mpui_slideshow_mode (slideshow, mode);
  slideshow->play = timer;
  slideshow->timer = timer;
  slideshow->next_timer = 0;
  slideshow->name_x = name_x;
  slideshow->name_y = name_y;
  slideshow->name_font = name_font;
  slideshow->border = mpui_obj_new (border, x, y, MPUI_FLAG_DYNAMIC,
                                    MPUI_DISPLAY_ALWAYS);
  slideshow->dirent = NULL;
  slideshow->dirent_size = 0;
  slideshow->need_generate = 1;

  return slideshow;
}

void
mpui_slideshow_free (mpui_slideshow_t *slideshow)
{
  mpui_slideshow_cleanup (slideshow);
  mpui_obj_free (slideshow->border);
  free (slideshow->name);
  mpui_container_uninit ((mpui_container_t *) slideshow);
  free (slideshow);
}


mpui_popup_t *
mpui_popup_new (char *id, mpui_coord_t x, mpui_coord_t y)
{
  mpui_popup_t *popup;

  popup = (mpui_popup_t *) malloc (sizeof (*popup));
  mpui_container_init ((mpui_container_t *) popup, id, MPUI_POPUP, 0, 
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
  mpui_list_free (popups->popups, (mpui_list_free_func_t) mpui_popup_free);
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
  mpui_list_free (screen->elements, (mpui_list_free_func_t) mpui_element_free);
  mpui_list_free (screen->popup_stack, (mpui_list_free_func_t) NULL);
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
  mpui_list_free (screens->screens, (mpui_list_free_func_t) mpui_screen_free);
  free (screens);
}

mpui_t *
mpui_new (int width, int height, int format, char *theme, char *lang)
{
  mpui_t *mpui;

  mpui = (mpui_t *) malloc (sizeof (*mpui));
  mpui->width = width;
  mpui->height = height;
  mpui->theme = mpui_strdup (theme);
  mpui->datadir = NULL;
  mpui->lang = mpui_strdup (lang);
  mpui->diag = sqrt (width*width + height*height);
  mpui->format = format;
  mpui->strings = NULL;
  mpui->fonts = NULL;
  mpui->images = mpui_images_new ();
  mpui->filetypes = mpui_list_new ();
  mpui->objects = mpui_objects_new ();
  mpui->menus = mpui_menus_new ();
  mpui->infos = mpui_infos_new ();
  mpui->popups = mpui_popups_new ();
  mpui->screens = NULL;
  mpui->current_screen = NULL;
  mpui->previous_screen = NULL;
  getcwd (mpui->cwd, sizeof (mpui->cwd));
  mpui->cwd[NAME_MAX] = '\0';
  mpui->lwd = NULL;
  mpui->cwd_id = 0;
  return mpui;
}

void
mpui_free (mpui_t *mpui)
{
  free (mpui->theme);
  free (mpui->datadir);
  free (mpui->lang);

  if (mpui->strings)
    mpui_strings_free (mpui->strings);

  if (mpui->fonts)
    mpui_fonts_free (mpui->fonts);

  if (mpui->images)
    mpui_images_free (mpui->images);

  mpui_list_free (mpui->filetypes, (mpui_list_free_func_t) mpui_filetypes_free);

  if (mpui->objects)
    mpui_objects_free (mpui->objects);

  if (mpui->menus)
    mpui_menus_free (mpui->menus);

  if (mpui->infos)
    mpui_infos_free (mpui->infos);

  if (mpui->popups)
    mpui_popups_free (mpui->popups);

  if (mpui->screens)
    mpui_screens_free (mpui->screens);
}
