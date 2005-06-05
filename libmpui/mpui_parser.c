/*
 *  mpui_parser.c: libmpui theme file parser.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <string.h>

#include "asxparser.h"
#include "config.h"

#include "mpui_struct.h"
#include "mpui_browser.h"
#include "mpui_playlist.h"
#include "mpui_focus.h"
#include "mpui_image.h"
#include "mpui_tv.h"
#include "mpui_cmd.h"
#include "mpui_parser.h"

extern char * get_path (char *filename);

char *
mpui_get_full_name (mpui_t *mpui, char *name)
{
  char *fullname = NULL;

  if (mpui->datadir)
    {
      fullname = (char *) malloc (strlen (mpui->datadir) + strlen (name) + 2);
      sprintf (fullname, "%s/%s", mpui->datadir, name);
    }
  else
    {
      struct stat st;
      char path[256];
      char *tmp, *datadir;

      snprintf (path, 256, "mpui/%s/%s", mpui->theme, name);
      fullname = get_path (path);

      /* try to get the full name of the file from home's dir first */
      if (!fullname || stat (fullname, &st))
        {
          /* ... then try from global data dir */
          int size = strlen (MPLAYER_DATADIR) + strlen (mpui->theme)
                     + strlen (name) + 8;
          if (fullname)
            fullname = (char *) realloc (fullname, size);
          else
            fullname = (char *) malloc (size);

          sprintf (fullname, "%s/mpui/%s/%s",
                   MPLAYER_DATADIR, mpui->theme, name);
          if (stat (fullname, &st))
            {
              free (fullname);
              return NULL;
            }
        }

      datadir = strdup (fullname);
      tmp = strrchr (datadir, '/');
      if (tmp)
        {
          *tmp = '\0';
          mpui->datadir = datadir;
        }
      else
        free (datadir);
    }

  return fullname;
}

static char *
mpui_read_filename (char *filename)
{
  int fd;
  ssize_t r;
  struct stat st;
  char *buffer;
  fd = open (filename, O_RDONLY);
  if (fd == -1)
    return NULL;
  if (fstat (fd, &st) == -1)
    {
      close (fd);
      return NULL;
    }
  buffer = (char *) malloc (st.st_size + 1);
  if (!buffer)
    return NULL;
  r = read (fd, buffer, st.st_size);
  close (fd);
  if (r != st.st_size)
    {
      free (buffer);
      return NULL;
    }
  buffer[r] = '\0';
  return buffer;
}

static int
mpui_parse_get_element (ASX_Parser_t* parser, char **buffer,
                        char **element, char **body, char ***attribs)
{
  int r;

  r = asx_get_element (parser, buffer, element, body, attribs);
  if (r < 0)
    {
      printf ("Syntax error at line %d\n", parser->line);
      asx_parser_free (parser);
    }
  else if (r == 0)
    {
      asx_parser_free (parser);
    }

  return r;
}

static mpui_when_focused_t
mpui_parse_when_focused (char **attribs)
{
  mpui_when_focused_t wf = MPUI_DISPLAY_ALWAYS;
  char *when_focused;

  when_focused = asx_get_attrib ("when-focused", attribs);

  if (when_focused)
    {
      if (!strcmp (when_focused, "yes"))
        wf = MPUI_DISPLAY_FOCUSED;
      else if (!strcmp (when_focused, "no"))
        wf = MPUI_DISPLAY_NORMAL;
      if (!strcmp (when_focused, "really-yes"))
        wf = MPUI_DISPLAY_REALLY_FOCUSED;
      if (!strcmp (when_focused, "really-no"))
        wf = MPUI_DISPLAY_REALLY_NORMAL;
    }

  free (when_focused);
  return wf;
}

static mpui_alignment_t
mpui_parse_alignment (char **attribs)
{
  mpui_alignment_t align = MPUI_ALIGNMENT_LEFT;
  char *al;

  al = asx_get_attrib ("align", attribs);

  if (al)
    {
      if (!strcmp (al, "top"))
        align = MPUI_ALIGNMENT_TOP;
      else if (!strcmp (al, "center"))
        align = MPUI_ALIGNMENT_CENTER;
      if (!strcmp (al, "right"))
        align = MPUI_ALIGNMENT_RIGHT;
      if (!strcmp (al, "bottom"))
        align = MPUI_ALIGNMENT_BOTTOM;
    }

  free (al);
  return align;
}

static mpui_coord_t
mpui_parse_size (char *size, int total, int diag, mpui_size_t deflt)
{
  mpui_coord_t coord;

  coord.val = deflt;
  coord.str = NULL;

  if (size && *size)
    {
      if (isdigit (size[0]) || (size[0] == '-' && isdigit (size[1])))
        {
          char *end;
          coord.val = strtol (size, &end, 10);
          if (*end == '%')
            coord.val = coord.val * total / 100;
          else if (*end == 'p')
            coord.val = coord.val * diag / 100;
        }
      else
        coord.str = strdup (size);
    }
  
  return coord;
}

static void
mpui_recompute_one_coord (mpui_coord_t *coord, mpui_size_t w, mpui_size_t h,
                          mpui_size_t total, mpui_size_t diag,
                          mpui_size_t center, int dynamic)
{
  struct { char *id; mpui_size_t v; } *l, list[] = {
    { "left",   0 },
    { "right",  w },
    { "top",    0 },
    { "bottom", h },
    { "width",  w },
    { "height", h },
    { "center", center},
    { NULL, 0 }
  };

  if (coord->str)
    {
      for (l=list; l->id; l++)
        if (!strncmp (coord->str, l->id, strlen (l->id)))
          {
            char *end, *s = coord->str + strlen (l->id);
            int val, mul = 1;
            if (*s && *s++ == '-')
              mul = -1;
            if (!isdigit (*s))
              {
                if (*s == '\0')
                  coord->val = l->v;
                break;
              }
            val = strtol (s, &end, 10);
            if (*end == '%')
              val = val * total / 100;
            else if (*end == 'p')
              val = val * diag / 100;
            coord->val = l->v + val * mul;
            break;
          }
      if (!dynamic)
        {
          free (coord->str);
          coord->str = NULL;
        }
    }
}

void
mpui_recompute_coord (mpui_t *mpui, mpui_element_t *element,
                      mpui_size_t w, mpui_size_t h, int dynamic,
                      int focus, int really_focus)
{
  mpui_element_t **elements = NULL;

  if (element->flags & MPUI_FLAG_DYNAMIC && !dynamic)
    return;
  if (dynamic && element->when_focused != MPUI_DISPLAY_ALWAYS
      && (!focus || element->when_focused != MPUI_DISPLAY_FOCUSED)
      && (!really_focus || element->when_focused !=MPUI_DISPLAY_REALLY_FOCUSED)
      && (really_focus || element->when_focused != MPUI_DISPLAY_NORMAL)
      && (focus || element->when_focused != MPUI_DISPLAY_REALLY_NORMAL))
    return;

  switch (element->type)
    {
    case MPUI_OBJ:
      if (element->flags & MPUI_FLAG_NOCOORD)
        {
          element->w.val = w;
          element->h.val = h;
        }
      break;

    case MPUI_STR:
    case MPUI_IMG:
      mpui_recompute_one_coord (&element->w, w, h, mpui->width, mpui->diag,
                                0, dynamic);
      mpui_recompute_one_coord (&element->h, w, h, mpui->height, mpui->diag,
                                0, dynamic);
      break;

    default:
      break;
    }

  mpui_recompute_one_coord (&element->x, w, h, mpui->width, mpui->diag,
                            (w-element->w.val)/2, dynamic);
  mpui_recompute_one_coord (&element->y, w, h, mpui->height, mpui->diag, 
                            (h-element->h.val)/2, dynamic);

  if (element->type == MPUI_IMG)
    mpui_img_load (mpui, (mpui_img_t *) element);

  if (element->flags & MPUI_FLAG_CONTAINER)
    for (elements=((mpui_container_t *) element)->elements;
         *elements; elements++)
      mpui_recompute_coord (mpui, *elements, element->w.val, element->h.val,
                            dynamic, focus, really_focus);

  element->flags &= ~MPUI_FLAG_NOCOORD;
}

static void
mpui_set_nocoord (mpui_element_t *element)
{
  if (element->x.str || element->y.str || element->h.str || element->w.str)
    element->flags |= MPUI_FLAG_NOCOORD;
}

void
mpui_clip (mpui_t *mpui, mpui_element_t *element,
           mpui_size_t x, mpui_size_t y, int dynamic)
{
  if (element->flags & MPUI_FLAG_DYNAMIC && !dynamic)
    return;

  if (element->flags & MPUI_FLAG_ABSOLUTE)
    {
      x = element->x.val;
      y = element->y.val;
    }
  else
    {
      x += element->x.val;
      y += element->y.val;
    }

  if (element->flags & MPUI_FLAG_CONTAINER)
    {
      mpui_element_t **elements = NULL;
      for (elements=((mpui_container_t *) element)->elements;
           *elements; elements++)
        if (element->type != MPUI_MNU
            || !((mpui_mnu_t *) element)->menu->is_browser
            || (*elements)->type != MPUI_MENUITEM)
          mpui_clip (mpui, *elements, x, y, dynamic);
    }
  else if (element->type == MPUI_IMG)
    {
      mpui_img_t *img = (mpui_img_t *) element;
      img->cx1 = x < 0 ? -x : 0;
      img->cy1 = y < 0 ? -y : 0;
      img->cx2 = x + img->element.w.val > mpui->width ?
                (x + img->element.w.val) - mpui->width : 0;
      img->cy2 = y + img->element.h.val > mpui->height ?
                (y + img->element.h.val) - mpui->height : 0;
    }
}

static mpui_color_t *
mpui_parse_color (char *color)
{
  if (color && color[0] == '#' && strlen (color) == 7)
    {
      char *end;
      int val = strtol (color+1, &end, 16);
      if (end > color+1 && *end == '\0')
        return mpui_color_new ((val & 0xFF0000) >> 16,
                               (val & 0x00FF00) >> 8,
                               (val & 0x0000FF));
    }

  return NULL;
}

static void
mpui_parse_node_keymap (mpui_keymaps_t *keymaps, char **attribs)
{
  char *key, *cmd;

  key = asx_get_attrib ("key", attribs);
  cmd = asx_get_attrib ("cmd", attribs);

  if (!key || !cmd)
    return;

  mpui_keymaps_add (keymaps, key, cmd);

  free (key);
  free (cmd);
}

static mpui_keymaps_t *
mpui_parse_node_keymaps (char *body, char **attribs)
{
  ASX_Parser_t* parser;
  char *id, *element, *sbody;
  mpui_keymaps_t *keymaps;

  id = asx_get_attrib ("id", attribs);
  asx_free_attribs (attribs);

  if (!id)
    return NULL;

  keymaps = mpui_keymaps_new (id);
  free (id);

  while (1)
    {
      parser = asx_parser_new();
      if (mpui_parse_get_element (parser,&body,&element,&sbody,&attribs) <= 0)
        break;

      if (!strcmp (element, "keymap"))
        mpui_parse_node_keymap (keymaps, attribs);

      asx_free_attribs (attribs);
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);

  return keymaps;
}

static void
mpui_parse_node_keymapping (mpui_t *mpui, char *body)
{
  ASX_Parser_t* parser;
  char *element, *sbody;
  char **attribs = NULL;

  while (1)
    {
      parser = asx_parser_new();
      if (mpui_parse_get_element (parser,&body,&element,&sbody,&attribs) <= 0)
        break;

      if (!strcmp (element, "keymaps"))
        {
          mpui_keymaps_t *keymaps = mpui_parse_node_keymaps (sbody, attribs);
          mpui_keymapping_add (mpui->keymapping, keymaps);
        }
      else
        asx_free_attribs (attribs);
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);
}

static mpui_string_t *
mpui_parse_node_string (mpui_t *mpui, char **attribs)
{
  char *id, *text, *file;
  mpui_string_t *string = NULL;

  id = asx_get_attrib ("id", attribs);
  text = asx_get_attrib ("text", attribs);
  file = asx_get_attrib ("file", attribs);

  if (id && !text && file)
    {
      char *f;

      f = mpui_get_full_name (mpui, file);
      text = mpui_read_filename (f);
      free (f);
      if (!text)
        {
          free (id);
          free (text);
          free (file);
          return NULL;
        }
    }

  if (id && text)
    string = mpui_string_new (id, text, MPUI_ENCODING_UTF8);

  asx_free_attribs (attribs);
  free (id);
  free (text);
  free (file);

  return string;
}

static mpui_str_t *
mpui_parse_node_str (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y, *absolute, *hidden;
  char *font_id, *size, *color, *focused_color, *really_focused_color;
  mpui_str_t *str = NULL;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  absolute = asx_get_attrib ("absolute", attribs);
  hidden = asx_get_attrib ("hidden", attribs);
  font_id = asx_get_attrib ("font", attribs);
  size = asx_get_attrib ("size", attribs);
  color = asx_get_attrib ("color", attribs);
  focused_color = asx_get_attrib ("focused-color", attribs);
  really_focused_color = asx_get_attrib ("really-focused-color", attribs);
  asx_free_attribs (attribs);

  if (id)
    {
      mpui_string_t *string;
      mpui_coord_t sx, sy;
      mpui_flags_t flags = 0;
      mpui_font_t *font = NULL;
      long s = MPUI_FONT_SIZE_DEFAULT;
      mpui_color_t *col, *fcol, *rfcol;
      mpui_when_focused_t wf;
      string = mpui_string_get (mpui, id);
      font = mpui_font_get (mpui, font_id);
      if (absolute && !strcmp (absolute, "yes"))
        flags |= MPUI_FLAG_ABSOLUTE;
      if (hidden && !strcmp (hidden, "yes"))
        flags |= MPUI_FLAG_HIDDEN;
      wf = mpui_parse_when_focused (attribs);
      if (size && *size)
        {
          char *endptr;
          s = strtol (size, &endptr, 0);
          if (*endptr)
            s = MPUI_FONT_SIZE_DEFAULT;
        }
      sx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
      sy = mpui_parse_size (y, mpui->height, mpui->diag, 0);
      col = mpui_parse_color (color);
      fcol = mpui_parse_color (focused_color);
      rfcol = mpui_parse_color (really_focused_color);

      if (string && font)
        {
          str = mpui_str_new (string, sx, sy, flags, font, s,
                              col, fcol, rfcol, wf);
          mpui_set_nocoord ((mpui_element_t *) str);
        }
    }

  free (id);
  free (x);
  free (y);
  free (absolute);
  free (hidden);
  free (font_id);
  free (size);
  free (color);
  free (focused_color);
  free (really_focused_color);

  return str;
}

static mpui_strings_t *
mpui_parse_node_strings (mpui_t *mpui, char **attribs, char *body)
{
  ASX_Parser_t* parser;
  char *fonts, *lang, *element;
  mpui_strings_t *strings;

  fonts = asx_get_attrib ("fonts", attribs);
  lang = asx_get_attrib ("lang", attribs);
  asx_free_attribs (attribs);

  if (!fonts || !lang)
    return NULL;

  /* do not care of <strings> from another language */
  if (strcmp (lang, mpui->lang) && *mpui->lang)
    {
      free (fonts);
      free (lang);
      return NULL;
    }

  strings = mpui_strings_new (fonts, lang);
  free (fonts);
  free (lang);

  while (1)
    {
      char *sbody;
      int res;

      parser = asx_parser_new();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "string"))
        {
          mpui_string_t *string = mpui_parse_node_string (mpui, attribs);
          mpui_strings_add (strings, string);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);
  return strings;
}

static mpui_image_t *
mpui_parse_node_image (mpui_t *mpui, char **attribs)
{
  char *id, *file, *f, *x, *y, *w, *h;
  mpui_coord_t sx, sy, sh, sw;
  mpui_image_t *image = NULL;

  id = asx_get_attrib ("id", attribs);
  file = asx_get_attrib ("file", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  w = asx_get_attrib ("w", attribs);
  h = asx_get_attrib ("h", attribs);

  sx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  sy = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  sw = mpui_parse_size (w, mpui->width, mpui->diag, 0);
  sh = mpui_parse_size (h, mpui->height, mpui->diag, 0);

  free (x);
  free (y);
  free (w);
  free (h);

  f = mpui_get_full_name (mpui, file);

  if (id && f)
    image = mpui_image_new (id, f, sx.val, sy.val, sw.val, sh.val);

  asx_free_attribs (attribs);
  free (id);
  free (file);
  free (f);

  return image;
}

static mpui_img_t *
mpui_parse_node_img (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y, *w, *h, *absolute, *hidden;
  mpui_img_t *img = NULL;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  w = asx_get_attrib ("w", attribs);
  h = asx_get_attrib ("h", attribs);
  absolute = asx_get_attrib ("absolute", attribs);
  hidden = asx_get_attrib ("hidden", attribs);

  if (id)
    {
      mpui_image_t *image;
      mpui_flags_t flags = 0;
      mpui_when_focused_t wf;

      image = mpui_image_get (mpui, id);
      if (absolute && !strcmp (absolute, "yes"))
        flags |= MPUI_FLAG_ABSOLUTE;
      if (hidden && !strcmp (hidden, "yes"))
        flags |= MPUI_FLAG_HIDDEN;
      wf = mpui_parse_when_focused (attribs);
      if (image)
        {
          mpui_coord_t sx, sy, sw, sh;
          sx = mpui_parse_size (x, mpui->width, mpui->diag, image->x);
          sy = mpui_parse_size (y, mpui->height, mpui->diag, image->y);
          sw = mpui_parse_size (w, mpui->width, mpui->diag, image->w);
          sh = mpui_parse_size (h, mpui->height, mpui->diag, image->h);
          img = mpui_img_new (image, sx, sy, sw, sh, flags, wf);
          mpui_set_nocoord ((mpui_element_t *) img);
        }
    }
  asx_free_attribs (attribs);
  free (id);
  free (x);
  free (y);
  free (w);
  free (h);
  free (absolute);
  free (hidden);
  return img;
}

static void
mpui_parse_node_images (mpui_t *mpui, char **attribs, char *body)
{
  ASX_Parser_t* parser;
  char *element;

  while (1)
    {
      char *sbody;
      int res;

      parser = asx_parser_new();      
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "image"))
        {
          mpui_image_t *image = mpui_parse_node_image (mpui, attribs);
          mpui_images_add (mpui->images, image);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);
}

static mpui_font_t *
mpui_parse_node_font (mpui_t *mpui, char **attribs)
{
  char *id, *file, *f, *size, *col, *focused_col, *really_focused_col;
  mpui_font_t *font = NULL;
  mpui_color_t *color, *focused_color, *really_focused_color;
  int s = MPUI_FONT_SIZE_DEFAULT;

  id = asx_get_attrib ("id", attribs);
  file = asx_get_attrib ("file", attribs);
  size = asx_get_attrib ("size", attribs);
  col = asx_get_attrib ("color", attribs);
  focused_col = asx_get_attrib ("focused-color", attribs);
  really_focused_col = asx_get_attrib ("really-focused-color", attribs);
  asx_free_attribs (attribs);
  
  if (size && *size)
    {
      char *endptr;
      s = strtol (size, &endptr, 0);
      if (*endptr)
        s = MPUI_FONT_SIZE_DEFAULT;
    }

  color = mpui_parse_color (col);
  focused_color = mpui_parse_color (focused_col);
  really_focused_color = mpui_parse_color (really_focused_col);

  f = mpui_get_full_name (mpui, file);

  if (id && file)
    font = mpui_font_new (mpui, id, f, s, color,
                          focused_color, really_focused_color);

  free (id);
  free (file);
  free (size);
  free (col);
  free (focused_col);
  free (really_focused_col);
  free (f);

  return font;
}

static mpui_fonts_t *
mpui_parse_node_fonts (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *dflt, *element;
  mpui_fonts_t *fonts;
  mpui_font_t **fnts;

  dflt = asx_get_attrib ("default", attribs);
  id = asx_get_attrib ("id", attribs);
  asx_free_attribs (attribs);

  if (!id || !dflt)
    return NULL;

  /* font id does not match the language fonts */
  if (!mpui->strings->fonts || strcmp (id, mpui->strings->fonts))
    {
      free (id);
      free (dflt);
      return NULL;
    }

  fonts = mpui_fonts_new ();

  while (1)
    {
      ASX_Parser_t* parser = asx_parser_new();
      char *sbody;
      int res;

      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "font"))
        {
          mpui_font_t *font = mpui_parse_node_font (mpui, attribs);
          mpui_fonts_add (fonts, font);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);

  for (fnts=fonts->fonts; *fnts; fnts++)
    if (!strcmp ((*fnts)->id, dflt))
      {
        fonts->deflt = *fnts;
        break;
      }

  free (id);
  free (dflt);

  return fonts;
}


static mpui_action_t *
mpui_parse_node_action (char **attribs)
{
  char *cmd, *when;
  mpui_action_t *action = NULL;
  mpui_action_when_t w = MPUI_WHEN_VALIDATE;
                  
  cmd = asx_get_attrib ("cmd", attribs);
  when = asx_get_attrib ("when", attribs);
  asx_free_attribs (attribs);

  if (when)
    {
      if (!strcmp (when, "focus"))
        w = MPUI_WHEN_FOCUS;
      else if (!strcmp (when, "unfocus"))
        w = MPUI_WHEN_UNFOCUS;
    }

  if (cmd)
    action = mpui_action_new (cmd, w);
         
  free (cmd);
  free (when);
  return action;
}


static mpui_filetype_t *
mpui_parse_node_filetype (mpui_t *mpui, char **attribs, char *body)
{
  mpui_filetype_t *filetype;
  mpui_match_t match = MPUI_MATCH_EXT;
  char *s_match;

  s_match = asx_get_attrib ("match", attribs);
  asx_free_attribs (attribs);

  if (s_match)
    {
      if (!strcmp (s_match, "all"))
        match = MPUI_MATCH_ALL;
      else if (!strcmp (s_match, "dir"))
        match = MPUI_MATCH_DIR;
    }

  filetype = mpui_filetype_new (match);

  while (1)
    {
      ASX_Parser_t* parser = asx_parser_new();
      char *element, *sbody;
      int res;

      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "action"))
        {
          mpui_action_t *action = mpui_parse_node_action (attribs);
          mpui_filetype_actions_add (filetype, action);
        }
      else if (!strcmp (element, "icon"))
        {
          char *id = asx_get_attrib ("id", attribs);
          filetype->icon = mpui_image_get (mpui, id);
          free (id);
        }
      else if (!strcmp (element, "ext") && sbody)
        mpui_filetype_exts_add (filetype, strdup (sbody));
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);

  free (s_match);

  return filetype;
}

static mpui_filetypes_t *
mpui_parse_node_filetypes (mpui_t *mpui, char **attribs, char *body)
{
  mpui_filetypes_t *filetypes;
  char *id;

  id = asx_get_attrib ("id", attribs);
  asx_free_attribs (attribs);
  filetypes = mpui_filetypes_new (id);

  while (1)
    {
      ASX_Parser_t* parser = asx_parser_new();
      char *element, *sbody;
      int res;

      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "file-type"))
        {
          mpui_filetype_t *ft = mpui_parse_node_filetype (mpui, attribs,sbody);
          mpui_filetypes_add (filetypes, ft);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);


  free (id);

  return filetypes;
}


static mpui_obj_t *
mpui_parse_node_obj (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y, *absolute, *hidden;
  mpui_obj_t *obj = NULL;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  absolute = asx_get_attrib ("absolute", attribs);
  hidden = asx_get_attrib ("hidden", attribs);

  if (id)
    {
      mpui_object_t *object;
      mpui_flags_t flags = 0;
      mpui_coord_t sx, sy;
      mpui_when_focused_t wf = MPUI_DISPLAY_ALWAYS;

      object = mpui_object_get (mpui, id);
      if (absolute && !strcmp (absolute, "yes"))
        flags |= MPUI_FLAG_ABSOLUTE;
      if (hidden && !strcmp (hidden, "yes"))
        flags |= MPUI_FLAG_HIDDEN;
      wf = mpui_parse_when_focused (attribs);

      if (object)
        {
          sx = mpui_parse_size (x, mpui->width, mpui->diag, object->x);
          sy = mpui_parse_size (y, mpui->height, mpui->diag, object->y);
          obj = mpui_obj_new (object, sx, sy, flags, wf);
        }
    }
  asx_free_attribs (attribs);
  free (id);
  free (x);
  free (y);
  free (absolute);
  free (hidden);
  return obj;
}

static mpui_object_t *
mpui_parse_node_object (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *x, *y, *keymaps_id, *element;
  mpui_coord_t sx, sy;
  mpui_object_t *object = NULL;
  mpui_keymaps_t *keymaps;
  ASX_Parser_t* parser;

  id = asx_get_attrib ("id", attribs);
  if (!id)
    return NULL;
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  keymaps_id = asx_get_attrib ("keymaps", attribs);
  sx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  sy = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  keymaps = mpui_keymaps_get (mpui, keymaps_id);
  object = mpui_object_new (id, sx.val, sy.val, keymaps);
  asx_free_attribs (attribs);
  free (id);
  free (x);
  free (y);
  free (keymaps_id);

  while (1)
    {
      mpui_element_t *elt = NULL;
      char *sbody;
      int res;

      parser = asx_parser_new();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "obj"))
        elt = (mpui_element_t *) mpui_parse_node_obj (mpui, attribs);
      else if (!strcmp (element, "img"))
        elt = (mpui_element_t *) mpui_parse_node_img (mpui, attribs);
      else if (!strcmp (element, "str"))
        elt = (mpui_element_t *) mpui_parse_node_str (mpui, attribs);
      else if (!strcmp (element, "action"))
        {
          mpui_action_t *action = mpui_parse_node_action (attribs);
          if (action)
            mpui_object_actions_add (object, action);
        }

      if (elt)
        mpui_object_elements_add (object, elt);
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);  
                  
  return object;
}

static void
mpui_parse_node_objects (mpui_t *mpui, char **attribs, char *body)
{
  char *element;
  ASX_Parser_t* parser;

  while (1)
    {
      char *sbody;
      int res;

      parser = asx_parser_new();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "object"))
        {
          mpui_object_t *object = mpui_parse_node_object (mpui, attribs, sbody);
          mpui_objects_add (mpui->objects, object);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);
}

static void
mpui_parse_node_channels (mpui_t *mpui, char **attribs, mpui_menu_t *menu,
                          mpui_size_t *mx, mpui_size_t *my,
                          mpui_size_t *mw, mpui_size_t *mh,
                          mpui_coord_t *spacing)
{
  char *mode, *empty_id;
  mpui_string_t *empty;

  mode = asx_get_attrib ("mode", attribs);
  if (!mode)
    return;
  empty_id = asx_get_attrib ("empty", attribs);
  empty = mpui_string_get (mpui, empty_id);
  free (empty_id);

  if (!empty)
    {
      free (mode);
      return;
    }

  if (!strcmp (mode, "tv"))
    mpui_tv_analog_channels_generate (menu, mx, my, mw, mh, spacing, empty);
  else if (!strcmp (mode, "dvb"))
    mpui_tv_dvb_channels_generate (menu, mx, my, mw, mh, spacing, empty);

  asx_free_attribs (attribs);
  free (mode);
}

static mpui_allmenuitem_t *
mpui_parse_node_menu_all_items (mpui_t *mpui, char **attribs, char *body,
                                mpui_menu_t *menu)
{
  mpui_allmenuitem_t *allmenuitem = NULL;
  mpui_container_t *container;
  ASX_Parser_t* parser;
  char *element;

  allmenuitem = mpui_allmenuitem_new (menu);
  container = &allmenuitem->container;

  while (1)
    {
      mpui_element_t *elt = NULL;
      char *sbody;
      int res;
      
      parser = asx_parser_new ();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;
              
      if (!strcmp (element, "obj"))
        elt = (mpui_element_t *) mpui_parse_node_obj (mpui, attribs);
      else if (!strcmp (element, "img"))
        elt = (mpui_element_t *) mpui_parse_node_img (mpui, attribs);
      else if (!strcmp (element, "str"))
        elt = (mpui_element_t *) mpui_parse_node_str (mpui, attribs);
      else if (!strcmp (element, "action"))
        {
          mpui_action_t *action = mpui_parse_node_action (attribs);
          if (action)
            mpui_container_actions_add (container, action);
        }

      if (elt)
        mpui_container_elements_add (container, elt);
      asx_parser_free (parser);
    }

  return allmenuitem;
}

static mpui_menuitem_t *
mpui_parse_node_menu_item (mpui_t *mpui, char **attribs, char *body)
{
  mpui_menuitem_t *menuitem = NULL;
  mpui_container_t *container;
  ASX_Parser_t* parser;
  char *element;

  menuitem = mpui_menuitem_new ();
  container = &menuitem->container;

  while (1)
    {
      mpui_element_t *elt = NULL;
      char *sbody;
      int res;
              
      parser = asx_parser_new ();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;
              
      if (!strcmp (element, "obj"))
        elt = (mpui_element_t *) mpui_parse_node_obj (mpui, attribs);
      else if (!strcmp (element, "img"))
        elt = (mpui_element_t *) mpui_parse_node_img (mpui, attribs);
      else if (!strcmp (element, "str"))
        elt = (mpui_element_t *) mpui_parse_node_str (mpui, attribs);
      else if (!strcmp (element, "action"))
        {
          mpui_action_t *action = mpui_parse_node_action (attribs);
          if (action)
            mpui_container_actions_add (container, action);
        }
      
      if (elt)
        mpui_container_elements_add (container, elt);
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);

  mpui_elements_get_size (&container->element, container->elements);

  return menuitem;
}

static mpui_menu_t *
mpui_parse_node_menu (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *orient, *spacing, *font_id, *x, *y, *w, *h, *keymaps_id, *element;
  ASX_Parser_t* parser;
  mpui_menu_t *menu = NULL;
  mpui_orientation_t orientation = MPUI_ORIENTATION_V;
  mpui_alignment_t align;
  mpui_font_t *default_font = NULL, *font = NULL;
  mpui_coord_t mx, my, ms;
  mpui_size_t item_x, item_y, max_w = 0, max_h = 0, tmp;
  mpui_keymaps_t *keymaps;
  mpui_element_t **elements, **e;
  mpui_size_t offset;

  id = asx_get_attrib ("id", attribs);
  orient = asx_get_attrib ("orientation", attribs);
  spacing = asx_get_attrib ("spacing", attribs);
  font_id = asx_get_attrib ("font", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  w = asx_get_attrib ("w", attribs);
  h = asx_get_attrib ("h", attribs);
  keymaps_id = asx_get_attrib ("keymaps", attribs);

  if (orient && !strcmp (orient, "horizontal"))
    orientation = MPUI_ORIENTATION_H;

  align = mpui_parse_alignment (attribs);

  if ((font = mpui_font_get (mpui, font_id)))
    {
      default_font = mpui->fonts->deflt;
      mpui->fonts->deflt = font;
    }

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  tmp = orientation == MPUI_ORIENTATION_V ? mpui->height : mpui->width;
  ms = mpui_parse_size (spacing, tmp, mpui->diag, 0);
  item_x = ms.val/2;
  item_y = ms.val/2;

  keymaps = mpui_keymaps_get (mpui, keymaps_id);

  if (id && font)
    menu = mpui_menu_new (id, orientation, mx.val, my.val, font, keymaps);

  asx_free_attribs (attribs);
  free (id);
  free (orient);
  free (spacing);
  free (font_id);
  free (x);
  free (y);
  free (keymaps_id);

  while (1)
    {
      mpui_element_t *elt = NULL;
      char *sbody;
      int res;
      
      parser = asx_parser_new ();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;
      
      if (!strcmp (element, "obj"))
        elt = (mpui_element_t *) mpui_parse_node_obj (mpui, attribs);
      else if (!strcmp (element, "img"))
        elt = (mpui_element_t *) mpui_parse_node_img (mpui, attribs);
      else if (!strcmp (element, "str"))
        elt = (mpui_element_t *) mpui_parse_node_str (mpui, attribs);
      else if (!strcmp (element, "menu-item"))
        {
          elt = (mpui_element_t *) mpui_parse_node_menu_item (mpui, attribs,
                                                              sbody);
          elt->x.val = item_x;
          elt->y.val = item_y;
          if (orientation == MPUI_ORIENTATION_V)
            {
              item_y += ms.val + elt->h.val;
              if (elt->w.val > max_w)
                max_w = elt->w.val;
            }
          else
            {
              item_x += ms.val + elt->w.val;
              if (elt->h.val > max_h)
                max_h = elt->h.val;
            }
        }
      else if (!strcmp (element, "all-menu-items"))
        elt = (mpui_element_t *) mpui_parse_node_menu_all_items (mpui, attribs,
                                                                 sbody, menu);
      else if (!strcmp (element, "channels"))
        mpui_parse_node_channels (mpui, attribs, menu,
                                  &item_x, &item_y, &max_w, &max_h, &ms);

      if (elt)
        mpui_menu_elements_add (menu, elt);
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);

  for (elements=menu->elements; *elements; elements++)
    if ((*elements)->type == MPUI_MENUITEM)
      {
        if (orientation == MPUI_ORIENTATION_V)
          {
            offset = max_w - (*elements)->w.val;
            switch (align)
              {
              case MPUI_ALIGNMENT_CENTER:
                offset >>= 1;
              case MPUI_ALIGNMENT_RIGHT:
                for (e=((mpui_container_t *) *elements)->elements; *e; e++)
                  if (!((*e)->flags & MPUI_FLAG_ABSOLUTE))
                    (*e)->x.val += offset;
              default:
                break;
              }
            (*elements)->w.val = max_w;
          }
        else
          {
            offset = max_h - (*elements)->h.val;
            switch (align)
              {
              case MPUI_ALIGNMENT_CENTER:
                offset >>= 1;
              case MPUI_ALIGNMENT_BOTTOM:
                for (e=((mpui_container_t *) *elements)->elements; *e; e++)
                  if (!((*e)->flags & MPUI_FLAG_ABSOLUTE))
                    (*e)->y.val += offset;
              default:
                break;
              }
            (*elements)->h.val = max_h;
          }
      }

  if (orientation == MPUI_ORIENTATION_V)
    item_x += max_w;
  else
    item_y += max_h;

  mx = mpui_parse_size (w, mpui->width, mpui->diag, item_x + ms.val/2);
  my = mpui_parse_size (h, mpui->height, mpui->diag, item_y + ms.val/2);
  menu->w = mx.val;
  menu->h = my.val;
  free (w);
  free (h);

  if (default_font)
    mpui->fonts->deflt = default_font;

  return menu;
}

static mpui_browser_t *
mpui_parse_node_browser (mpui_t *mpui, char **attribs)
{
  char *id, *orient, *scroll, *spacing, *font_id, *x, *y, *w, *h, *i_w, *i_h;
  char *border_id, *item_border_id, *filter_id, *keymaps_id;
  mpui_browser_t *browser = NULL;
  mpui_orientation_t orientation = MPUI_ORIENTATION_V, scrolling;
  mpui_alignment_t align;
  mpui_font_t *font;
  mpui_object_t *border, *item_border;
  mpui_filetypes_t *filter;
  mpui_keymaps_t *keymaps;
  mpui_coord_t mx, my, mw, mh, miw, mih, ms;
  mpui_size_t item_w, tmp;

  id = asx_get_attrib ("id", attribs);
  orient = asx_get_attrib ("orientation", attribs);
  scroll = asx_get_attrib ("scrolling", attribs);
  spacing = asx_get_attrib ("spacing", attribs);
  font_id = asx_get_attrib ("font", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  w = asx_get_attrib ("w", attribs);
  h = asx_get_attrib ("h", attribs);
  i_w = asx_get_attrib ("icon-w", attribs);
  i_h = asx_get_attrib ("icon-h", attribs);
  border_id = asx_get_attrib ("border", attribs);
  item_border_id = asx_get_attrib ("item-border", attribs);
  filter_id = asx_get_attrib ("filter", attribs);
  keymaps_id = asx_get_attrib ("keymaps", attribs);

  if (orient && !strcmp (orient, "both"))
    orientation = MPUI_ORIENTATION_H | MPUI_ORIENTATION_V;
  else if (orient && !strcmp (orient, "horizontal"))
    orientation = MPUI_ORIENTATION_H;

  scrolling = orientation;
  if (scroll)
    {
      if (!strcmp (scroll, "vertical"))
        scrolling = MPUI_ORIENTATION_V;
      else if (!strcmp (scroll, "horizontal"))
        scrolling = MPUI_ORIENTATION_H;
    }
  if (scrolling == (MPUI_ORIENTATION_H | MPUI_ORIENTATION_V))
    scrolling = MPUI_ORIENTATION_V;

  align = mpui_parse_alignment (attribs);

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  mw = mpui_parse_size (w, mpui->width, mpui->diag, 0);
  mh = mpui_parse_size (h, mpui->height, mpui->diag, 0);
  miw = mpui_parse_size (i_w, mpui->width, mpui->diag, 0);
  mih = mpui_parse_size (i_h, mpui->height, mpui->diag, 0);
  tmp = orientation == MPUI_ORIENTATION_V ? mpui->height : mpui->width;
  ms = mpui_parse_size (spacing, tmp, mpui->diag, 0);

  font = mpui_font_get (mpui, font_id);
  filter = mpui_filetypes_get (mpui, filter_id);
  keymaps = mpui_keymaps_get (mpui, keymaps_id);
  if (id && font && filter)
    {
      if (!i_w && !i_h)
        mih.val = miw.val = orientation == MPUI_ORIENTATION_V ?
                        font->font_desc->height : 2 * font->font_desc->height;
      else if (!i_w)
        miw.val = mih.val;
      else if (!i_h)
        mih.val = miw.val;

      if (orientation & MPUI_ORIENTATION_H)
        {
          item_w = font->font_desc->height * 8;
          item_w = (mw.val / ((mw.val / (item_w + ms.val)) + 1)) - ms.val;
          if (item_w < miw.val)
            item_w = (mw.val / (mw.val / (miw.val + ms.val))) - ms.val;
        }
      else
        item_w = mw.val;

      border = mpui_object_get (mpui, border_id);
      item_border = mpui_object_get (mpui, item_border_id);

      filter = mpui_filetypes_dup (filter, miw.val, mih.val);
      browser = mpui_browser_new (id, font, orientation, scrolling, align,
                                  mx.val, my.val, mw.val, mh.val,
                                  miw.val, mih.val, item_w, ms.val,
                                  border, item_border, filter, keymaps);
      mpui_browser_generate (mpui, browser);
    }

  asx_free_attribs (attribs);
  free (id);
  free (orient);
  free (scroll);
  free (spacing);
  free (font_id);
  free (x);
  free (y);
  free (w);
  free (h);
  free (i_w);
  free (i_h);
  free (border_id);
  free (item_border_id);
  free (filter_id);
  free (keymaps_id);

  return browser;
}

static mpui_playlist_t *
mpui_parse_node_playlist (mpui_t *mpui, char **attribs)
{
  char *id, *orient, *scroll, *spacing, *font_id, *x, *y, *w, *h;
  char *border_id, *item_border_id, *keymaps_id;
  mpui_playlist_t *playlist = NULL;
  mpui_orientation_t orientation = MPUI_ORIENTATION_V, scrolling;
  mpui_alignment_t align;
  mpui_font_t *font;
  mpui_object_t *border, *item_border;
  mpui_keymaps_t *keymaps;
  mpui_coord_t mx, my, mw, mh, ms;
  mpui_size_t item_w, tmp;

  id = asx_get_attrib ("id", attribs);
  orient = asx_get_attrib ("orientation", attribs);
  scroll = asx_get_attrib ("scrolling", attribs);
  spacing = asx_get_attrib ("spacing", attribs);
  font_id = asx_get_attrib ("font", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  w = asx_get_attrib ("w", attribs);
  h = asx_get_attrib ("h", attribs);
  border_id = asx_get_attrib ("border", attribs);
  item_border_id = asx_get_attrib ("item-border", attribs);
  keymaps_id = asx_get_attrib ("keymaps", attribs);

  if (orient && !strcmp (orient, "both"))
    orientation = MPUI_ORIENTATION_H | MPUI_ORIENTATION_V;
  else if (orient && !strcmp (orient, "horizontal"))
    orientation = MPUI_ORIENTATION_H;

  scrolling = orientation;
  if (scroll)
    {
      if (!strcmp (scroll, "vertical"))
        scrolling = MPUI_ORIENTATION_V;
      else if (!strcmp (scroll, "horizontal"))
        scrolling = MPUI_ORIENTATION_H;
    }
  if (scrolling == (MPUI_ORIENTATION_H | MPUI_ORIENTATION_V))
    scrolling = MPUI_ORIENTATION_V;

  align = mpui_parse_alignment (attribs);

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  mw = mpui_parse_size (w, mpui->width, mpui->diag, 0);
  mh = mpui_parse_size (h, mpui->height, mpui->diag, 0);
  tmp = orientation == MPUI_ORIENTATION_V ? mpui->height : mpui->width;
  ms = mpui_parse_size (spacing, tmp, mpui->diag, 0);
  font = mpui_font_get (mpui, font_id);

  keymaps = mpui_keymaps_get (mpui, keymaps_id);

  if (id && font)
    {
      if (orientation & MPUI_ORIENTATION_H)
        {
          item_w = font->font_desc->height * 8;
          item_w = (mw.val / ((mw.val / (item_w + ms.val)) + 1)) - ms.val;
        }
      else
        item_w = mw.val;

      border = mpui_object_get (mpui, border_id);
      item_border = mpui_object_get (mpui, item_border_id);

      playlist = mpui_playlist_new (id, font, orientation, scrolling, align,
                                    mx.val, my.val, mw.val, mh.val,
                                    item_w, ms.val, border, item_border,
                                    keymaps);
      mpui_playlist_generate (playlist);
    }

  asx_free_attribs (attribs);
  free (id);
  free (orient);
  free (scroll);
  free (spacing);
  free (font_id);
  free (x);
  free (y);
  free (w);
  free (h);
  free (border_id);
  free (item_border_id);
  free (keymaps_id);

  return playlist;
}

static mpui_slideshow_t *
mpui_parse_node_slideshow (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y, *w, *h, *path, *filter_id, *mode, *timer, *border_id;
  char *name_x, *name_y, *name_font_id;
  mpui_coord_t sx, sy, sw, sh, nx, ny;
  mpui_filetypes_t *filter;
  mpui_font_t *name_font = NULL;
  mpui_object_t *border = NULL;
  mpui_slideshow_t *slideshow = NULL;
  int t = 0;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  w = asx_get_attrib ("w", attribs);
  h = asx_get_attrib ("h", attribs);
  path = asx_get_attrib ("path", attribs);
  filter_id = asx_get_attrib ("filter", attribs);
  mode = asx_get_attrib ("mode", attribs);
  timer = asx_get_attrib ("timer", attribs);
  name_x = asx_get_attrib ("name_x", attribs);
  name_y = asx_get_attrib ("name_y", attribs);
  name_font_id = asx_get_attrib ("name_font", attribs);
  border_id = asx_get_attrib ("border", attribs);

  sx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  sy = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  sw = mpui_parse_size (w, mpui->width, mpui->diag, 0);
  sh = mpui_parse_size (h, mpui->height, mpui->diag, 0);
  nx = mpui_parse_size (name_x, mpui->width, mpui->diag, -1);
  ny = mpui_parse_size (name_y, mpui->height, mpui->diag, 0);

  filter = mpui_filetypes_get (mpui, filter_id);
  if (name_y)
    name_font = mpui_font_get (mpui, name_font_id);

  if (timer)
    {
      char *endptr;
      t = strtol (timer, &endptr, 0);
      if (*endptr)
        t = 0;
    }

  if (border_id)
    border = mpui_object_get (mpui, border_id);

  if (id && filter)
    slideshow = mpui_slideshow_new (id, sx, sy, sw, sh, path, filter, mode, t,
                                    nx, ny, name_font, border);

  free (id);
  free (x);
  free (y);
  free (w);
  free (h);
  free (path);
  free (filter_id);
  free (mode);
  free (timer);
  free (name_x);
  free (name_y);
  free (name_font_id);
  free (border_id);

  return slideshow;
}

static mpui_tag_t *
mpui_parse_node_tag (mpui_t *mpui, char **attribs)
{
  char *id, *caption, *type, *x, *y;
  mpui_tag_t *tag = NULL;
  mpui_string_t *string = NULL;
  mpui_coord_t mx, my;

  id = asx_get_attrib ("id", attribs);
  caption = asx_get_attrib ("caption", attribs);
  type = asx_get_attrib ("type", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);

  if (caption)
    string = mpui_string_get (mpui, caption);

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);

  if (id && string && type)
    tag = mpui_tag_new (id, string->text, type, mx, my);

  asx_free_attribs (attribs);
  free (id);
  free (caption);
  free (type);
  free (x);
  free (y);

  return tag;
}

static mpui_pic_t *
mpui_parse_node_pic (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y, *w, *h, *filter_id;
  mpui_pic_t *pic = NULL;
  mpui_coord_t mx, my, mw, mh;
  mpui_filetypes_t *filter;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  w = asx_get_attrib ("w", attribs);
  h = asx_get_attrib ("h", attribs);
  filter_id = asx_get_attrib ("filter", attribs);

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  mw = mpui_parse_size (w, mpui->height, mpui->diag, 0);
  mh = mpui_parse_size (h, mpui->height, mpui->diag, 0);

  filter = mpui_filetypes_get (mpui, filter_id);

  if (id && filter)
    pic = mpui_pic_new (id, mx, my, mw, mh, filter);

  asx_free_attribs (attribs);
  free (id);
  free (x);
  free (y);
  free (w);
  free (h);
  free (filter_id);

  return pic;
}

static mpui_sys_t *
mpui_parse_node_sys (mpui_t *mpui, char **attribs)
{
  char *id, *caption, *type, *x, *y;
  mpui_sys_t *sys = NULL;
  mpui_string_t *string = NULL;
  mpui_coord_t mx, my;

  id = asx_get_attrib ("id", attribs);
  caption = asx_get_attrib ("caption", attribs);
  type = asx_get_attrib ("type", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);

  if (caption)
    string = mpui_string_get (mpui, caption);

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);

  if (id && string && type)
    sys = mpui_sys_new (id, string->text, type, mx, my);

  asx_free_attribs (attribs);
  free (id);
  free (caption);
  free (type);
  free (x);
  free (y);

  return sys;
}

static mpui_inf_t *
mpui_parse_node_inf (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y, *absolute, *hidden, *timer;
  mpui_inf_t *inf = NULL;
  unsigned int t = 0;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  absolute = asx_get_attrib ("absolute", attribs);
  hidden = asx_get_attrib ("hidden", attribs);
  timer = asx_get_attrib ("timer", attribs);

  if (id)
    {
      mpui_info_t *info;
      mpui_flags_t flags = 0;
      mpui_coord_t sx, sy;
      mpui_when_focused_t wf = MPUI_DISPLAY_ALWAYS;

      info = mpui_info_get (mpui, id);
      if (absolute && !strcmp (absolute, "yes"))
        flags |= MPUI_FLAG_ABSOLUTE;
      if (hidden && !strcmp (hidden, "yes"))
        flags |= MPUI_FLAG_HIDDEN;
      wf = mpui_parse_when_focused (attribs);

      if (timer)
        {
          char *endptr;
          t = strtol (timer, &endptr, 0);
          if (*endptr)
            t = 0;
        }

      if (info)
        {
          sx = mpui_parse_size (x, mpui->width, mpui->diag, info->x.val);
          sy = mpui_parse_size (y, mpui->height, mpui->diag, info->y.val);
          inf = mpui_inf_new (info, sx, sy, wf, t);
        }
    }
  asx_free_attribs (attribs);
  free (id);
  free (x);
  free (y);
  free (absolute);
  free (hidden);

  return inf;
}

static mpui_info_t *
mpui_parse_node_info (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *font_id, *x, *y, *w, *h, *element;
  ASX_Parser_t* parser;
  mpui_info_t *info = NULL;
  mpui_font_t *font;
  mpui_coord_t mx, my, mw, mh;

  id = asx_get_attrib ("id", attribs);
  font_id = asx_get_attrib ("font", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  w = asx_get_attrib ("w", attribs);
  h = asx_get_attrib ("h", attribs);
  asx_free_attribs (attribs);

  font = mpui_font_get (mpui, font_id);

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  mw = mpui_parse_size (w, mpui->width, mpui->diag, 0);
  mh = mpui_parse_size (h, mpui->height, mpui->diag, 0);

  if (id && font)
    info = mpui_info_new (id, font, mx, my, mw, mh);

  free (id);
  free (font_id);
  free (x);
  free (y);
  free (w);
  free (h);

  if (info)
    while (1)
      {
        char *sbody;
        int res;
        
        parser = asx_parser_new ();
        res = mpui_parse_get_element (parser,&body,&element,&sbody,&attribs);
        if (res <= 0)
          break;
        
        if (!strcmp (element, "tag"))
          {
            mpui_tag_t *tag = mpui_parse_node_tag (mpui, attribs);
            if (tag)
              mpui_info_add_tag (info, tag);
          }
        else if (!strcmp (element, "pic"))
          {
            mpui_pic_t *pic = mpui_parse_node_pic (mpui, attribs);
            if (pic)
              mpui_info_add_pic (info, pic);
          }
        else if (!strcmp (element, "sys"))
          {
            mpui_sys_t *sys = mpui_parse_node_sys (mpui, attribs);
            if (sys)
              mpui_info_add_sys (info, sys);
          }
        asx_parser_free (parser);
      }
  asx_free_attribs (attribs);

  return info;
}

static void
mpui_parse_node_infos (mpui_t *mpui, char **attribs, char *body)
{
  char *element, *sbody;
  ASX_Parser_t* parser;
  int res;

  asx_free_attribs (attribs);

  while (1)
    {
      parser = asx_parser_new ();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "info"))
        {
          mpui_info_t *info;
          info = mpui_parse_node_info (mpui, attribs, sbody);
          
          if (info)
            mpui_infos_add (mpui->infos, info);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);
}

static mpui_mnu_t *
mpui_parse_node_mnu (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y, *absolute, *hidden;
  mpui_mnu_t *mnu = NULL;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  absolute = asx_get_attrib ("absolute", attribs);
  hidden = asx_get_attrib ("hidden", attribs);
  asx_free_attribs (attribs);

  if (id)
    {
      mpui_menu_t *menu = mpui_menu_get (mpui, id);
      mpui_flags_t flags = 0;
      mpui_coord_t sx, sy;
      if (absolute && !strcmp (absolute, "yes"))
        flags |= MPUI_FLAG_ABSOLUTE;
      if (hidden && !strcmp (hidden, "yes"))
        flags |= MPUI_FLAG_HIDDEN;
      if (menu)
        {
          sx = mpui_parse_size (x, mpui->width, mpui->diag, menu->x);
          sy = mpui_parse_size (y, mpui->height, mpui->diag, menu->y);
          mnu = mpui_mnu_new (menu, sx, sy, flags);
        }
    }

  free (id);
  free (x);
  free (y);
  free (absolute);
  free (hidden);
  return mnu;
}

static void
mpui_parse_node_menus (mpui_t *mpui, char **attribs, char *body)
{
  ASX_Parser_t* parser;
  char *element;

  while (1)
    {
      char *sbody;
      int res;

      parser = asx_parser_new ();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;
      
      if (!strcmp (element, "menu"))
        {
          mpui_menu_t *menu = mpui_parse_node_menu (mpui, attribs, sbody);
          mpui_menus_add (mpui->menus, menu);
        }
      else if (!strcmp (element, "browser"))
        {
          mpui_browser_t *browser = mpui_parse_node_browser (mpui, attribs);
          mpui_menus_add (mpui->menus, (mpui_menu_t *) browser);
        }
      else if (!strcmp (element, "playlist"))
        {
          mpui_playlist_t *playlist = mpui_parse_node_playlist (mpui, attribs);
          mpui_menus_add (mpui->menus, (mpui_menu_t *) playlist);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);
}

static mpui_popup_t *
mpui_parse_node_popup (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *sx, *sy, *keymaps_id, *element;
  ASX_Parser_t* parser;
  mpui_popup_t *popup = NULL;
  mpui_container_t *container;
  mpui_keymaps_t *keymaps;
  mpui_coord_t x, y;
  
  id = asx_get_attrib ("id", attribs);
  sx = asx_get_attrib ("x", attribs);
  sy = asx_get_attrib ("y", attribs);
  keymaps_id = asx_get_attrib ("keymaps", attribs);
  x = mpui_parse_size (sx, mpui->width, mpui->diag, 0);
  y = mpui_parse_size (sy, mpui->height, mpui->diag, 0);
  keymaps = mpui_keymaps_get (mpui, keymaps_id);
  asx_free_attribs (attribs);
  free (sx);
  free (sy);
  free (keymaps_id);

  if (id)
    {
      popup = mpui_popup_new (id, x, y, keymaps);
      mpui_set_nocoord ((mpui_element_t *) popup);
      container = (mpui_container_t *) popup;

      while (1)
        {
          mpui_element_t *elt = NULL;
          char *sbody;
          int res;

          parser = asx_parser_new ();
          res = mpui_parse_get_element (parser,&body,&element,&sbody,&attribs);
          if (res <= 0)
            break;

          if (!strcmp (element, "obj"))
            elt = (mpui_element_t *) mpui_parse_node_obj (mpui, attribs);
          else if (!strcmp (element, "img"))
            elt = (mpui_element_t *) mpui_parse_node_img (mpui, attribs);
          else if (!strcmp (element, "str"))
            elt = (mpui_element_t *) mpui_parse_node_str (mpui, attribs);
          else if (!strcmp (element, "mnu"))
            elt = (mpui_element_t *) mpui_parse_node_mnu (mpui, attribs);
          else if (!strcmp (element, "inf"))
            elt = (mpui_element_t *) mpui_parse_node_inf (mpui, attribs);
          else if (!strcmp (element, "slideshow"))
            elt = (mpui_element_t *) mpui_parse_node_slideshow (mpui, attribs);

          if (elt)
            mpui_container_elements_add (container, elt);
          asx_parser_free (parser);
        }
      asx_free_attribs (attribs);
      mpui_elements_get_size (&container->element, container->elements);
    }

  free (id);
  return popup;
}

static void
mpui_parse_node_popups (mpui_t *mpui, char **attribs, char *body)
{
  char *element, *sbody;
  ASX_Parser_t* parser;
  int res;

  asx_free_attribs (attribs);

  while (1)
    {
      parser = asx_parser_new ();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "popup"))
        {
          mpui_popup_t *popup = mpui_parse_node_popup (mpui, attribs, sbody);
          mpui_popups_add (mpui->popups, popup);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);
}

static mpui_screen_t *
mpui_parse_node_screen (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *keymaps_id, *element;
  ASX_Parser_t* parser;
  mpui_screen_t *screen = NULL;
  mpui_keymaps_t *keymaps;
  
  id = asx_get_attrib ("id", attribs);
  keymaps_id = asx_get_attrib ("keymaps", attribs);
  asx_free_attribs (attribs);

  keymaps = mpui_keymaps_get (mpui, keymaps_id);
  if (id)
    screen = mpui_screen_new (id, keymaps);
  free (id);
  free (keymaps_id);

  while (1)
    {
      mpui_element_t *elt = NULL;
      char *sbody;
      int res;

      parser = asx_parser_new ();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "obj"))
        elt = (mpui_element_t *) mpui_parse_node_obj (mpui, attribs);
      else if (!strcmp (element, "img"))
        elt = (mpui_element_t *) mpui_parse_node_img (mpui, attribs);
      else if (!strcmp (element, "str"))
        elt = (mpui_element_t *) mpui_parse_node_str (mpui, attribs);
      else if (!strcmp (element, "mnu"))
        elt = (mpui_element_t *) mpui_parse_node_mnu (mpui, attribs);
      else if (!strcmp (element, "inf"))
        elt = (mpui_element_t *) mpui_parse_node_inf (mpui, attribs);
      else if (!strcmp (element, "slideshow"))
        elt = (mpui_element_t *) mpui_parse_node_slideshow (mpui, attribs);

      if (elt && screen)
        mpui_screen_elements_add (screen, elt);
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);

  if (screen)
    mpui_focus_box_first (screen);

  return screen;
}

static mpui_screens_t *
mpui_parse_node_screens (mpui_t *mpui, char **attribs, char *body)
{
  char *menu, *control, *element;
  ASX_Parser_t* parser;
  mpui_screens_t *screens;

  menu = asx_get_attrib ("menu", attribs);
  control = asx_get_attrib ("control", attribs);
  asx_free_attribs (attribs);

  screens = mpui_screens_new ();

  while (1)
    {
      char *sbody;
      int res;

      parser = asx_parser_new ();
      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "screen"))
        {
          mpui_screen_t *screen = mpui_parse_node_screen (mpui, attribs, sbody);
          mpui_screens_add (screens, screen);
        }
      asx_parser_free (parser);
    }
  asx_free_attribs (attribs);

  screens->menu = mpui_screen_get (screens, menu);
  screens->ctrl = mpui_screen_get (screens, control);

  free (menu);
  free (control);
  return screens;
}

static int
mpui_parse_config (mpui_t *mpui, char *buffer,
                   int width, int height, int format)
{
  char *element, *body, **attribs;
  mpui_element_t **elements;
  ASX_Parser_t* parser = asx_parser_new();

  if (mpui_parse_get_element (parser, &buffer, &element, &body, &attribs) < 0)
    return -1;

  if (strcmp (element, "mpui"))
    {
      printf ("Not a valid ui config file %d\n", parser->line);
      asx_parser_free (parser);
      return -1;
    }

  while(1)
    {
      char *sbody;
      int r;

      r = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (r < 0)
        return -1;
      else if (r == 0)
        break;

      if (!strcmp (element, "include"))
        {
          char *file, *f, *buffer;

          file = asx_get_attrib ("file", attribs);
          if (!file)
            continue;

          f = mpui_get_full_name (mpui, file);
          free (file);
          if (!f)
            continue;

          buffer = mpui_read_filename (f);
          free (f);
          if (!buffer)
            continue;

          mpui_parse_config (mpui, buffer, width, height, format);
          free (buffer);
        }
      else if (!strcmp (element, "keymapping"))
        {
          mpui_parse_node_keymapping (mpui, sbody);
        }
      else if (!mpui->strings && !strcmp (element, "strings"))
        {
          mpui->strings = mpui_parse_node_strings (mpui, attribs, sbody);
        }
      else if (!strcmp (element, "images"))
        {
          mpui_parse_node_images (mpui, attribs, sbody);
        }
      else if (!mpui->fonts && !strcmp (element, "fonts"))
        {
          mpui->fonts = mpui_parse_node_fonts (mpui, attribs, sbody);
        }
      else if (!strcmp (element, "file-types"))
        {
          mpui_filetypes_t *ft = mpui_parse_node_filetypes(mpui,attribs,sbody);
          mpui_filetypes_list_add (mpui, ft);
        }
      else if (!strcmp (element, "objects"))
        {
          mpui_parse_node_objects (mpui, attribs, sbody);
        }
      else if (!strcmp (element, "menus"))
        {
          mpui_parse_node_menus (mpui, attribs, sbody);
        }
      else if (!strcmp (element, "popups"))
        {
          mpui_parse_node_popups (mpui, attribs, sbody);
        }
      else if (!strcmp (element, "infos"))
        {
          mpui_parse_node_infos (mpui, attribs, sbody);
        }
      else if (mpui->screens == NULL && !strcmp (element, "screens"))
        {
          mpui_screens_t *screens;
          screens = mpui_parse_node_screens (mpui, attribs, sbody);
          mpui->screens = screens;
        }

      free (element);
    }

  if (mpui->screens)
    {
      mpui_screen_t **screen;
      mpui->current_screen = mpui->screens->menu;
      mpui_cmd_screen_set_keymaps (mpui, mpui->current_screen);
      for (screen=mpui->screens->screens; *screen; screen++)
        for (elements=(*screen)->elements; *elements; elements++)
          {
            mpui_recompute_coord (mpui, *elements,
                                  mpui->width, mpui->height, 0, 0, 0);
            mpui_clip (mpui, *elements, 0, 0, 0);
          }
    }
  if (mpui->popups)
    {
      mpui_popup_t **popup;
      for (popup=mpui->popups->popups; *popup; popup++)
        {
          mpui_recompute_coord (mpui, (mpui_element_t *) *popup,
                                mpui->width, mpui->height, 0, 0, 0);
          mpui_clip (mpui, (mpui_element_t *) *popup, 0, 0, 0);
        }
    }

  return 0;
}

mpui_t *
mpui_parse_config_file (char *theme, char *lang,
                        int width, int height, int format)
{
  char *buffer, *filename;
  mpui_t *mpui;

  mpui = mpui_new (width, height, format, theme, lang);  
  filename = mpui_get_full_name (mpui, "mpui.conf");
  if (!filename)
    return NULL;

  buffer = mpui_read_filename (filename);
  free (filename);
  if (!buffer)
    return NULL;

  if (mpui_parse_config (mpui, buffer, width, height, format))
    {
      mpui_free (mpui);
      free (buffer);
      return NULL;
    }

  free (buffer);
  return mpui;
}
