/*
 *  mpui_parser.c: libmpui theme file parser.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "../asxparser.h"

#include "mpui_struct.h"
#include "mpui_parser.h"

int
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

mpui_string_t *
mpui_parse_node_string (char **attribs)
{
  char *id, *text;
  mpui_string_t *string = NULL;

  id = asx_get_attrib ("id", attribs);
  text = asx_get_attrib ("text", attribs);

  if (id && text)
    string = mpui_string_new (id, text);
  asx_free_attribs (attribs);

  return string;
}

mpui_str_t *
mpui_parse_node_str (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y;
  char *font_id, *size, *color, *focused_color, *when_focused;
  mpui_str_t *str = NULL;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  font_id = asx_get_attrib ("font", attribs);
  size = asx_get_attrib ("size", attribs);
  color = asx_get_attrib ("color", attribs);
  focused_color = asx_get_attrib ("focused_color", attribs);
  when_focused = asx_get_attrib ("when_focused", attribs);

  if (id)
    {
      mpui_string_t *string;
      mpui_size_t sx, sy;
      mpui_font_t *font;
      long s = MPUI_FONT_SIZE_DEFAULT;
      mpui_color_t col, fcol;
      int wf = MPUI_DISPLAY_ALWAYS;
      string = mpui_string_get (mpui, id);
      font = mpui_font_get (mpui, font_id);
      if (when_focused)
        {
          if (!strcmp (when_focused, "yes"))
            wf = MPUI_DISPLAY_FOCUSED;
          else if (!strcmp (when_focused, "no"))
            wf = MPUI_DISPLAY_NORMAL;
        }
      if (size && *size)
        {
          char *endptr;
          s = strtol (size, &endptr, 0);
          if (*endptr)
            s = MPUI_FONT_SIZE_DEFAULT;
        }
      /* FIXME: get sx, sy, and colors */
      if (string)
        str = mpui_str_new (string, sx, sy, font, s, col, fcol, wf);
    }
  asx_free_attribs (attribs);

  printf ("Str attrib id =  %s\n", id);
  printf ("Str attrib x =  %s\n", x);
  printf ("Str attrib y =  %s\n", y);
  printf ("Str attrib font =  %s\n", font_id);
  printf ("Str attrib size =  %s\n", size);
  printf ("Str attrib color =  %s\n", color);
  printf ("Str attrib focused_color =  %s\n", focused_color);
  printf ("Str attrib when_focused =  %s\n", when_focused);

  return str;
}

mpui_strings_t *
mpui_parse_node_strings (char **attribs, char *body)
{
  ASX_Parser_t* parser;
  char *code, *lang, *element;
  mpui_strings_t *strings;

  code = asx_get_attrib ("encoding", attribs);
  lang = asx_get_attrib ("lang", attribs);
  asx_free_attribs (attribs);

  strings = mpui_strings_new (code, lang);
  printf ("Strings attrib encoding =  %s\n", code);
  printf ("Strings attrib lang =  %s\n", lang);

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

mpui_image_t *
mpui_parse_node_image (char **attribs)
{
  char *id, *file, *x, *y, *h, *w;
  mpui_image_t *image = NULL;

  id = asx_get_attrib ("id", attribs);
  file = asx_get_attrib ("file", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  h = asx_get_attrib ("h", attribs);
  w = asx_get_attrib ("w", attribs);

  if (id && file)
    image = mpui_image_new (id, file, x, y, h, w);
  asx_free_attribs (attribs);

  return image;
}

mpui_img_t *
mpui_parse_node_img (mpui_t *mpui, char **attribs)
{
  char *id, *x, *y, *h, *w, *when_focused;
  mpui_img_t *img = NULL;

  id = asx_get_attrib ("id", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  h = asx_get_attrib ("h", attribs);
  w = asx_get_attrib ("w", attribs);
  when_focused = asx_get_attrib ("when_focused", attribs);

  if (id)
    {
      mpui_image_t *image;
      mpui_size_t sx, sy, sh, sw;
      int wf = MPUI_DISPLAY_ALWAYS;

      image = mpui_image_get (mpui, id);
      if (when_focused)
        {
          if (!strcmp (when_focused, "yes"))
            wf = MPUI_DISPLAY_FOCUSED;
          else if (!strcmp (when_focused, "no"))
            wf = MPUI_DISPLAY_NORMAL;
        }
      /* FIXME: get sx, sy, sh and sw */
      if (image)
        img = mpui_img_new (image, sx, sy, sh, sw, wf);
    }
  asx_free_attribs (attribs);

  printf ("Img attrib id =  %s\n", id);
  printf ("Img attrib x =  %s\n", x);
  printf ("Img attrib y =  %s\n", y);
  printf ("Img attrib h =  %s\n", h);
  printf ("Img attrib w =  %s\n", w);
  printf ("Img attrib when_focused =  %s\n", when_focused);

  return img;
}

mpui_images_t *
mpui_parse_node_images (char **attribs, char *body)
{
  ASX_Parser_t* parser;
  char *element;
  mpui_images_t *images;

  images = mpui_images_new ();

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
          mpui_image_t *image = mpui_parse_node_image (attribs);
          mpui_images_add (images, image);
        }
      free (parser);
    }
  asx_free_attribs (attribs);
  return images;
}

void
mpui_parse_node_font (char **attribs)
{
  char *id, *file, *size, *color, *focused_color;
                  
  id = asx_get_attrib ("id", attribs);
  file = asx_get_attrib ("file", attribs);
  size = asx_get_attrib ("size", attribs);
  color = asx_get_attrib ("color", attribs);
  focused_color = asx_get_attrib ("focused-color", attribs);
  asx_free_attribs (attribs);
                  
  printf ("Font attrib id =  %s\n", id);
  printf ("Font attrib file =  %s\n", file);
  printf ("Font attrib size =  %s\n", size);
  printf ("Font attrib color =  %s\n", color);
  printf ("Font attrib focused-color =  %s\n", focused_color);
}

void
mpui_parse_node_fonts (char **attribs, char *body)
{
  char *dflt, *element;

  dflt = asx_get_attrib ("default", attribs);
  asx_free_attribs (attribs);

  printf ("Fonts attrib default =  %s\n", dflt);

  while (1)
    {
      ASX_Parser_t* parser = asx_parser_new();
      char *sbody;
      int res;

      res = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (res <= 0)
        break;

      if (!strcmp (element, "font"))
        mpui_parse_node_font (attribs);
      free (parser);
    }
  asx_free_attribs (attribs);
}


void
mpui_parse_node_action (char **attribs)
{
  char *cmd;
                  
  cmd = asx_get_attrib ("cmd", attribs);
  asx_free_attribs (attribs);
                  
  printf ("Action attrib cmd =  %s\n", cmd);
}


void
mpui_parse_node_object (char **attribs)
{
  char *id, *relative, *dynamic, *when_focused;
                  
  id = asx_get_attrib ("id", attribs);
  relative = asx_get_attrib ("relative", attribs);
  dynamic = asx_get_attrib ("dynamic", attribs);
  when_focused = asx_get_attrib ("when-focused", attribs);
  asx_free_attribs (attribs);
                  
  printf ("Object attrib id =  %s\n", id);
  printf ("Object attrib relative =  %s\n", relative);
  printf ("Object attrib dynamic =  %s\n", dynamic);
  printf ("Object attrib when-focused =  %s\n", when_focused);
}

void
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
          ASX_Parser_t* sparser;

          mpui_parse_node_object (attribs);
              
          while (1)
            {
              char *ssbody;
              int sres;

              sparser = asx_parser_new();
              sres = mpui_parse_get_element (sparser, &sbody, &element,
                                             &ssbody, &attribs);
              if (sres <= 0)
                break;

              if (!strcmp (element, "obj"))
                mpui_parse_node_object (attribs);
              else if (!strcmp (element, "img"))
                mpui_parse_node_img (mpui, attribs);
              else if (!strcmp (element, "str"))
                mpui_parse_node_str (mpui, attribs);
              else if (!strcmp (element, "action"))
                mpui_parse_node_action (attribs);
              free (sparser);
            }
          asx_free_attribs (attribs);  
        }
      free (parser);
    }
  asx_free_attribs (attribs);
}


void
mpui_parse_node_menu (char **attribs)
{
  char *id, *orientation, *font, *x, *y, *h, *w;
                  
  id = asx_get_attrib ("id", attribs);
  orientation = asx_get_attrib ("orientation", attribs);
  font = asx_get_attrib ("font", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  h = asx_get_attrib ("h", attribs);
  w = asx_get_attrib ("w", attribs);
  asx_free_attribs (attribs);
                  
  printf ("Menu attrib id =  %s\n", id);
  printf ("Menu attrib orientation =  %s\n", orientation);
  printf ("Menu attrib font =  %s\n", font);
  printf ("Menu attrib x =  %s\n", x);
  printf ("Menu attrib y =  %s\n", y);
  printf ("Menu attrib h =  %s\n", h);
  printf ("Menu attrib w =  %s\n", w);
}

void
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
          ASX_Parser_t* sparser;

          mpui_parse_node_menu (attribs);
              
          while (1)
            {
              char *ssbody;
              int sres;

              sparser = asx_parser_new ();
              sres = mpui_parse_get_element (sparser, &sbody, &element,
                                             &ssbody, &attribs);
              if (sres <= 0)
                break;

              if (!strcmp (element, "obj"))
                mpui_parse_node_object (attribs);
              else if (!strcmp (element, "img"))
                mpui_parse_node_image (attribs);
              else if (!strcmp (element, "str"))
                mpui_parse_node_str (mpui, attribs);
              else if (!strcmp (element, "all-menu-items"))
                {
                  ASX_Parser_t* ssparser;

                  while (1)
                    {
                      char *sssbody;
                      int ssres;

                      ssparser = asx_parser_new ();
                      ssres = mpui_parse_get_element (ssparser, &ssbody,
                                                      &element, &sssbody,
                                                      &attribs);
                      if (ssres <= 0)
                        break;

                      if (!strcmp (element, "obj"))
                        mpui_parse_node_object (attribs);
                      else if (!strcmp (element, "img"))
                        mpui_parse_node_image (attribs);
                      else if (!strcmp (element, "str"))
                        mpui_parse_node_str (mpui, attribs);
                      else if (!strcmp (element, "action"))
                        mpui_parse_node_action (attribs);
                      free (ssparser);
                    }
                }
              else if (!strcmp (element, "menu-item"))
                {
                  ASX_Parser_t* ssparser;

                  while (1)
                    {
                      char *sssbody;
                      int ssres;

                      ssparser = asx_parser_new ();
                      ssres = mpui_parse_get_element (ssparser, &ssbody,
                                                      &element, &sssbody,
                                                      &attribs);
                      if (ssres <= 0)
                        break;

                      if (!strcmp (element, "obj"))
                        mpui_parse_node_object (attribs);
                      else if (!strcmp (element, "img"))
                        mpui_parse_node_image (attribs);
                      else if (!strcmp (element, "str"))
                        mpui_parse_node_str (mpui, attribs);
                      else if (!strcmp (element, "action"))
                        mpui_parse_node_action (attribs);
                      free (ssparser);
                    }
                  asx_free_attribs (attribs);
                }
              free (sparser);
            }
          asx_free_attribs (attribs);
        }
      free (parser);
    }
  asx_free_attribs (attribs);
}

void
mpui_parse_node_screen (char **attribs)
{
  char *id;
                  
  id = asx_get_attrib ("id", attribs);
  asx_free_attribs (attribs);
                  
  printf ("Screen attrib id =  %s\n", id);
}

void
mpui_parse_node_screens (mpui_t *mpui, char **attribs, char *body)
{
  char *menu, *control, *element;
  ASX_Parser_t* parser;

  menu = asx_get_attrib ("menu", attribs);
  control = asx_get_attrib ("control", attribs);
  asx_free_attribs (attribs);

  printf ("Screens attrib menu =  %s\n", menu);
  printf ("Screens attrib control =  %s\n", control);

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
          ASX_Parser_t* sparser;

          mpui_parse_node_screen (attribs);
              
          while (1)
            {
              char *ssbody;
              int sres;

              sparser = asx_parser_new ();
              sres = mpui_parse_get_element (sparser, &sbody, &element,
                                             &ssbody, &attribs);
              if (sres <= 0)
                break;

              if (!strcmp (element, "obj"))
                mpui_parse_node_object (attribs);
              else if (!strcmp (element, "img"))
                mpui_parse_node_image (attribs);
              else if (!strcmp (element, "str"))
                mpui_parse_node_str (mpui, attribs);
              else if (!strcmp (element, "mnu"))
                mpui_parse_node_menu (attribs);
              free (sparser);
            }
          asx_free_attribs (attribs);
        }
      free (parser);
    }
  asx_free_attribs (attribs);
}


mpui_t *
mpui_parse_config (char *buffer)
{
  char *element, *body, **attribs;
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

  mpui = mpui_new ();
  printf ("***** Element root : %s\n", element);

  while(1)
    {
      char *sbody;
      int r;

      r = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (r < 0)
        return NULL;
      else if (r == 0)
        return mpui;

      printf ("***** Element type : %s\n", element);
      if (!strcmp (element, "strings"))
        {
          mpui_strings_t *strings = mpui_parse_node_strings (attribs, sbody);
          mpui_strings_add (mpui, strings);
        }
      else if (!strcmp (element, "images"))
        {
          mpui_images_t *images = mpui_parse_node_images (attribs, sbody);
          mpui_images_add (mpui, images);
        }
      else if (!strcmp (element, "fonts"))
        mpui_parse_node_fonts (attribs, sbody);
      else if (!strcmp (element, "objects"))
        mpui_parse_node_objects (mpui, attribs, sbody);
      else if (!strcmp (element, "menus"))
        mpui_parse_node_menus (mpui, attribs, sbody);
      else if (!strcmp (element, "screens"))
        mpui_parse_node_screens (mpui, attribs, sbody);

      free (element);
    }
  free (parser);
  return mpui;
}

mpui_t *
mpui_parse_config_file (char *filename)
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
  mpui = mpui_parse_config (buffer);
  free (buffer);

  return mpui;
}
