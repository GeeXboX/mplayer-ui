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

#include "asxparser.h"
#include "config.h"

#include "mpui_struct.h"
#include "mpui_browser.h"
#include "mpui_focus.h"
#include "mpui_image.h"
#include "mpui_parser.h"

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
      if (isdigit (size[0]))
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

static mpui_string_t *
mpui_parse_node_string (char **attribs)
{
  char *id, *text, *file;
  mpui_string_t *string = NULL;

  id = asx_get_attrib ("id", attribs);
  text = asx_get_attrib ("text", attribs);
  file = asx_get_attrib ("file", attribs);

  if (id && !text && file)
    {
      int fd, r;
      struct stat st;
      char *f;

      f = (char *) malloc (strlen (MPUI_DATADIR) + strlen (file) + 1);
      snprintf (f, strlen (MPUI_DATADIR) + strlen (file) + 1,
                "%s%s", MPUI_DATADIR, file);

      fd = open (f, O_RDONLY);
      free (f);
      if (fd == -1)
        {
          free (id);
          free (text);
          free (file);
          return NULL;
        }
      if (fstat (fd, &st) == -1)
        {
          close (fd);
          free (id);
          free (text);
          free (file);
          return NULL;
        }
      text = (char *) malloc (st.st_size);
      r = read (fd, text, st.st_size);
      close (fd);
      if (r != st.st_size)
        {
          free (id);
          free (text);
          free (file);
          return NULL;
        }
    }

  if (id && text)
    string = mpui_string_new (id, text);

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
mpui_parse_node_strings (char **attribs, char *body)
{
  ASX_Parser_t* parser;
  char *code, *lang, *element;
  mpui_strings_t *strings;

  code = asx_get_attrib ("encoding", attribs);
  lang = asx_get_attrib ("lang", attribs);
  asx_free_attribs (attribs);

  strings = mpui_strings_new (code, lang);
  free (code);
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
          mpui_string_t *string = mpui_parse_node_string (attribs);
          mpui_strings_add (strings, string);
        }
      free (parser);
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

  f = (char *) malloc (strlen (MPUI_DATADIR) + strlen (file) + 1);
  snprintf (f, strlen (MPUI_DATADIR) + strlen (file) + 1,
            "%s%s", MPUI_DATADIR, file);

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
      free (parser);
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

  f = (char *) malloc (strlen (MPUI_DATADIR) + strlen (file) + 1);
  snprintf (f, strlen (MPUI_DATADIR) + strlen (file) + 1,
            "%s%s", MPUI_DATADIR, file);

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
  char *dflt, *element;
  mpui_fonts_t *fonts;
  mpui_font_t **fnts;

  dflt = asx_get_attrib ("default", attribs);
  asx_free_attribs (attribs);

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
      free (parser);
    }
  asx_free_attribs (attribs);

  for (fnts=fonts->fonts; *fnts; fnts++)
    if (!strcmp ((*fnts)->id, dflt))
      {
        fonts->deflt = *fnts;
        break;
      }

  return fonts;
}


static mpui_action_t *
mpui_parse_node_action (char **attribs)
{
  char *cmd;
  mpui_action_t *action = NULL;
                  
  cmd = asx_get_attrib ("cmd", attribs);
  asx_free_attribs (attribs);

  if (cmd)
    action = mpui_action_new (cmd);
         
  free (cmd);
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
          mpui_actions_add (filetype, action);
        }
      else if (!strcmp (element, "icon"))
        {
          char *id = asx_get_attrib ("id", attribs);
          filetype->icon = mpui_image_get (mpui, id);
          free (id);
        }
      else if (!strcmp (element, "ext") && sbody)
        mpui_ext_add (filetype, strdup (sbody));
      free (parser);
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
      free (parser);
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
  char *id, *x, *y, *element;
  mpui_coord_t sx, sy;
  mpui_object_t *object = NULL;
  ASX_Parser_t* parser;

  id = asx_get_attrib ("id", attribs);
  if (!id)
    return NULL;
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  sx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  sy = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  object = mpui_object_new (id, sx.val, sy.val);
  asx_free_attribs (attribs);
  free (id);
  free (x);
  free (y);

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
            mpui_actions_add (object, action);
        }

      if (elt)
        mpui_add_element (object, elt);
      free (parser);
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
      free (parser);
    }
  asx_free_attribs (attribs);
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
            mpui_actions_add (container, action);
        }

      if (elt)
        mpui_add_element (container, elt);
      free (parser);
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
            mpui_actions_add (container, action);
        }
      
      if (elt)
        mpui_add_element (container, elt);
      free (parser);
    }
  asx_free_attribs (attribs);

  mpui_elements_get_size (&container->element, container->elements);

  return menuitem;
}

static mpui_menu_t *
mpui_parse_node_menu (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *orient, *spacing, *font_id, *x, *y, *w, *h, *element;
  ASX_Parser_t* parser;
  mpui_menu_t *menu = NULL;
  mpui_orientation_t orientation = MPUI_ORIENTATION_V;
  mpui_alignment_t align;
  mpui_font_t *default_font = NULL, *font = NULL;
  mpui_coord_t mx, my, ms;
  mpui_size_t item_x, item_y, max_w = 0, max_h = 0, tmp;
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

  if (orient && !strcmp (orient, "horizontal"))
    orientation = MPUI_ORIENTATION_H;

  align = mpui_parse_alignment (attribs);

  if (font_id && (font = mpui_font_get (mpui, font_id)))
    {
      default_font = mpui->fonts[0]->deflt;
      mpui->fonts[0]->deflt = font;
    }

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  tmp = orientation == MPUI_ORIENTATION_V ? mpui->height : mpui->width;
  ms = mpui_parse_size (spacing, tmp, mpui->diag, 0);
  item_x = ms.val/2;
  item_y = ms.val/2;

  if (id)
    menu = mpui_menu_new (id, orientation, mx.val, my.val);

  asx_free_attribs (attribs);
  free (id);
  free (orient);
  free (spacing);
  free (font_id);
  free (x);
  free (y);

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

      if (elt)
        mpui_add_element (menu, elt);
      free (parser);
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

  if (font)
    mpui->fonts[0]->deflt = default_font;

  return menu;
}

static mpui_browser_t *
mpui_parse_node_browser (mpui_t *mpui, char **attribs)
{
  char *id, *orient, *scroll, *spacing, *font_id, *x, *y, *w, *h, *i_w, *i_h;
  char *border_id, *item_border_id, *filter_id;
  mpui_browser_t *browser = NULL;
  mpui_orientation_t orientation = MPUI_ORIENTATION_V, scrolling;
  mpui_alignment_t align;
  mpui_font_t *font;
  mpui_object_t *border, *item_border;
  mpui_filetypes_t *filter;
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

  if (!font_id || !(font = mpui_font_get (mpui, font_id)))
    font = mpui->fonts[0]->deflt;

  mx = mpui_parse_size (x, mpui->width, mpui->diag, 0);
  my = mpui_parse_size (y, mpui->height, mpui->diag, 0);
  mw = mpui_parse_size (w, mpui->width, mpui->diag, 0);
  mh = mpui_parse_size (h, mpui->height, mpui->diag, 0);
  miw = mpui_parse_size (i_w, mpui->width, mpui->diag, 0);
  mih = mpui_parse_size (i_h, mpui->height, mpui->diag, 0);
  tmp = orientation == MPUI_ORIENTATION_V ? mpui->height : mpui->width;
  ms = mpui_parse_size (spacing, tmp, mpui->diag, 0);

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

  filter = mpui_filetypes_get (mpui, filter_id);
  if (id && filter)
    {
      filter = mpui_filetypes_dup (filter, miw.val, mih.val);
      browser = mpui_browser_new (id, font, orientation, scrolling, align,
                                  mx.val, my.val, mw.val, mh.val,
                                  miw.val, mih.val, item_w, ms.val,
                                  border, item_border, filter);
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

  return browser;
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
          mpui_menus_add (mpui->menus, browser);
        }
      free (parser);
    }
  asx_free_attribs (attribs);
}

static mpui_popup_t *
mpui_parse_node_popup (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *sx, *sy, *element;
  ASX_Parser_t* parser;
  mpui_popup_t *popup = NULL;
  mpui_container_t *container;
  mpui_coord_t x, y;
  
  id = asx_get_attrib ("id", attribs);
  sx = asx_get_attrib ("x", attribs);
  sy = asx_get_attrib ("y", attribs);
  x = mpui_parse_size (sx, mpui->width, mpui->diag, 0);
  y = mpui_parse_size (sy, mpui->height, mpui->diag, 0);
  asx_free_attribs (attribs);
  free (sx);
  free (sy);

  if (id)
    {
      popup = mpui_popup_new (id, x, y);
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

          if (elt)
            mpui_add_element (container, elt);
          free (parser);
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
      free (parser);
    }
  asx_free_attribs (attribs);
}

static mpui_screen_t *
mpui_parse_node_screen (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *element;
  ASX_Parser_t* parser;
  mpui_screen_t *screen = NULL;
  
  id = asx_get_attrib ("id", attribs);
  asx_free_attribs (attribs);

  if (id)
    screen = mpui_screen_new (id);
  free (id);

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

      if (elt && screen)
        mpui_add_element (screen, elt);
      free (parser);
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
      free (parser);
    }
  asx_free_attribs (attribs);

  screens->menu = mpui_screen_get (screens, menu);
  screens->ctrl = mpui_screen_get (screens, control);

  free (menu);
  free (control);
  return screens;
}

extern char * get_path (char *filename);

mpui_t *
mpui_parse_config (mpui_t *ui, char *buffer, int width, int height, int format)
{
  char *element, *body, **attribs;
  mpui_element_t **elements;
  mpui_t* mpui;
  ASX_Parser_t* parser = asx_parser_new();

  if (mpui_parse_get_element (parser, &buffer, &element, &body, &attribs) < 0)
    return NULL;

  if (strcmp (element, "mpui"))
    {
      printf ("Not a valid ui config file %d\n", parser->line);
      asx_parser_free (parser);
      return NULL;
    }

  if (ui)
    mpui = ui;
  else
    mpui = mpui_new (width, height, format);

  while(1)
    {
      char *sbody;
      int r;

      r = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (r < 0)
        return NULL;
      else if (r == 0)
        break;

      if (!strcmp (element, "include"))
        {
          int fd, r;
          struct stat st;
          char *buffer;
          char *file, *f;
          
          file = asx_get_attrib ("file", attribs);
          f = get_path (file);

          fd = open (f, O_RDONLY);
          if (fd == -1)
            {
              f = (char *) malloc (strlen (MPLAYER_CONFDIR)
                                   + strlen (file) + 2);
              snprintf (f, strlen (MPLAYER_CONFDIR) + strlen (file) + 2,
                        "%s/%s", MPLAYER_CONFDIR, file);
              fd = open (f, O_RDONLY);
              free (f);
            }
          if (fd == -1)
            continue;
          if (fstat (fd, &st) == -1)
            {
              close (fd);
              continue;
            }
          buffer = (char *) malloc (st.st_size);
          r = read (fd, buffer, st.st_size);
          close (fd);
          if (r != st.st_size)
            {
              free (buffer);
              continue;
            }
          mpui_parse_config (mpui, buffer, width, height, format);
          free (buffer);
        }
      else if (!strcmp (element, "strings"))
        {
          mpui_strings_t *strings = mpui_parse_node_strings (attribs, sbody);
          mpui_strings_add (mpui, strings);
        }
      else if (!strcmp (element, "images"))
        {
          mpui_parse_node_images (mpui, attribs,sbody);
        }
      else if (!strcmp (element, "fonts"))
        {
          mpui_fonts_t *fonts = mpui_parse_node_fonts (mpui, attribs, sbody);
          mpui_fonts_add (mpui, fonts);
        }
      else if (!strcmp (element, "file-types"))
        {
          mpui_filetypes_t *ft = mpui_parse_node_filetypes(mpui,attribs,sbody);
          mpui_filetypes_add (mpui, ft);
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
      else if (!strcmp (element, "screens"))
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

  return mpui;
}

mpui_t *
mpui_parse_config_file (char *filename, int width, int height, int format)
{
  int fd, r;
  struct stat st;
  char *buffer;
  mpui_t *mpui;

  fd = open (filename, O_RDONLY);
  if (fd == -1)
    return NULL;
  if (fstat (fd, &st) == -1)
    {
      close (fd);
      return NULL;
    }
  buffer = (char *) malloc (st.st_size);
  r = read (fd, buffer, st.st_size);
  close (fd);
  if (r != st.st_size)
    {
      free (buffer);
      return NULL;
    }
  mpui = mpui_parse_config (NULL, buffer, width, height, format);
  free (buffer);

  return mpui;
}
