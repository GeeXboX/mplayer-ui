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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

mpui_size_t
mpui_parse_size (char *size)
{
  mpui_size_t st;

  st.val = 0;
  st.absolute = 1;
  if (size && *size)
    {
      /* Determine absolute position/size */
      if (isdigit (size[0]))
        {
          char *end;
          st.val = strtol (size, &end, 10);
          st.absolute = (*end == '%') ? 0 : 1;
        }
      /* FIXME: determine position/size for special keywords like
         left, right, top, bottom, width, height */
    }
  
  return st;
}

mpui_color_t
mpui_parse_color (char *color)
{
  mpui_color_t c = {0, 0, 0};
  
  if (color && color[0] == '#' && strlen (color) == 7)
    {
      char *end;
      int val = strtol (color+1, &end, 16);
      if (end > color+1 && *end == '\0')
        {
          c.r = (val & 0xFF0000) >> 16;
          c.g = (val & 0x00FF00) >> 8;
          c.b = (val & 0x0000FF);
        }
    }

  return c;
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
      sx = mpui_parse_size (x);
      sy = mpui_parse_size (y);
      col = mpui_parse_color (color);
      fcol = mpui_parse_color (focused_color);

      if (string)
        str = mpui_str_new (string, sx, sy, font, s, col, fcol, wf);
    }
  asx_free_attribs (attribs);

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
  mpui_size_t sx, sy, sh, sw;
  mpui_image_t *image = NULL;

  id = asx_get_attrib ("id", attribs);
  file = asx_get_attrib ("file", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  h = asx_get_attrib ("h", attribs);
  w = asx_get_attrib ("w", attribs);

  sx = mpui_parse_size (x);
  sy = mpui_parse_size (y);
  sh = mpui_parse_size (h);
  sw = mpui_parse_size (w);

  if (id && file)
    image = mpui_image_new (id, file, sx, sy, sh, sw);
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
      sx = mpui_parse_size (x);
      sy = mpui_parse_size (y);
      sh = mpui_parse_size (h);
      sw = mpui_parse_size (w);
      if (image)
        img = mpui_img_new (image, sx, sy, sh, sw, wf);
    }
  asx_free_attribs (attribs);

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

mpui_font_t *
mpui_parse_node_font (char **attribs)
{
  char *id, *file, *size, *col, *focused_col;
  mpui_font_t *font = NULL;
  mpui_color_t color, focused_color;
  int s = MPUI_FONT_SIZE_DEFAULT;

  id = asx_get_attrib ("id", attribs);
  file = asx_get_attrib ("file", attribs);
  size = asx_get_attrib ("size", attribs);
  col = asx_get_attrib ("color", attribs);
  focused_col = asx_get_attrib ("focused-color", attribs);
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

  if (id && file)
    font = mpui_font_new (id, file, s, color, focused_color);

  return font;
}

mpui_fonts_t *
mpui_parse_node_fonts (mpui_t *mpui, char **attribs, char *body)
{
  char *dflt, *element;
  mpui_fonts_t *fonts;

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
          mpui_font_t *font = mpui_parse_node_font (attribs);
          mpui_fonts_add (fonts, font);
        }
      free (parser);
    }
  asx_free_attribs (attribs);

  fonts->deflt = mpui_font_get (mpui, dflt);

  return fonts;
}


mpui_action_t *
mpui_parse_node_action (char **attribs)
{
  char *cmd;
  mpui_action_t *action = NULL;
                  
  cmd = asx_get_attrib ("cmd", attribs);
  asx_free_attribs (attribs);

  if (cmd)
    action = mpui_action_new (cmd);
                  
  return action;
}

mpui_obj_t *
mpui_parse_node_obj (mpui_t *mpui, char **attribs)
{
  char *id, *when_focused;
  mpui_obj_t *obj = NULL;

  id = asx_get_attrib ("id", attribs);
  when_focused = asx_get_attrib ("when_focused", attribs);

  if (id)
    {
      mpui_object_t *object;
      int wf = MPUI_DISPLAY_ALWAYS;

      object = mpui_object_get (mpui, id);
      if (when_focused)
        {
          if (!strcmp (when_focused, "yes"))
            wf = MPUI_DISPLAY_FOCUSED;
          else if (!strcmp (when_focused, "no"))
            wf = MPUI_DISPLAY_NORMAL;
        }

      if (object)
        obj = mpui_obj_new (object, wf);
    }
  asx_free_attribs (attribs);

  return obj;
}

mpui_object_t *
mpui_parse_node_object (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *relative, *dynamic, *element;
  mpui_object_t *object = NULL;
  mpui_object_flags_t flags = 0;
  ASX_Parser_t* parser;

  id = asx_get_attrib ("id", attribs);
  relative = asx_get_attrib ("relative", attribs);
  dynamic = asx_get_attrib ("dynamic", attribs);

  /* FIXME: setting args hangs MPlayer */
  /*   if (!strcmp (relative, "yes")) */
  /*     flags |= MPUI_OBJECT_RELATIVE; */
  /*   if (!strcmp (dynamic, "yes")) */
  /*     flags |= MPUI_OBJECT_DYNAMIC; */

  if (id)
    object = mpui_object_new (id, flags);
  asx_free_attribs (attribs);

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
        {
          mpui_obj_t *obj = mpui_parse_node_obj (mpui, attribs);
          elt = mpui_element_new (MPUI_OBJ, obj);
        }
      else if (!strcmp (element, "img"))
        {
          mpui_img_t *img = mpui_parse_node_img (mpui, attribs);
          elt = mpui_element_new (MPUI_IMG, img);
        }
      else if (!strcmp (element, "str"))
        {
          mpui_str_t *str = mpui_parse_node_str (mpui, attribs);
          elt = mpui_element_new (MPUI_STR, str);
        }
      else if (!strcmp (element, "action"))
        {
          mpui_action_t *action = mpui_parse_node_action (attribs);
          elt = mpui_element_new (MPUI_ACTION, action);
        }

      if (elt)
        mpui_add_element (object, elt);
      free (parser);
    }
  asx_free_attribs (attribs);  
                  
  return object;
}

mpui_objects_t *
mpui_parse_node_objects (mpui_t *mpui, char **attribs, char *body)
{
  char *element;
  ASX_Parser_t* parser;
  mpui_objects_t *objects;

  objects = mpui_objects_new ();

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
          mpui_objects_add (objects, object);
        }
      free (parser);
    }
  asx_free_attribs (attribs);

  return objects;
}

mpui_allmenuitem_t *
mpui_parse_node_menu_all_items (mpui_t *mpui, char **attribs, char *body)
{
  mpui_allmenuitem_t *allmenuitem = NULL;
  ASX_Parser_t* parser;
  char *element;

  allmenuitem = mpui_allmenuitem_new ();
          
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
        {
          mpui_obj_t *obj = mpui_parse_node_obj (mpui, attribs);
          elt = mpui_element_new (MPUI_OBJ, obj);
        }
      else if (!strcmp (element, "img"))
        {
          mpui_img_t *img = mpui_parse_node_img (mpui, attribs);
          elt = mpui_element_new (MPUI_IMG, img);
        }
      else if (!strcmp (element, "str"))
        {
          mpui_str_t *str = mpui_parse_node_str (mpui, attribs);
          elt = mpui_element_new (MPUI_STR, str);
        }
      else if (!strcmp (element, "action"))
        {
          mpui_action_t *action = mpui_parse_node_action (attribs);
          elt = mpui_element_new (MPUI_ACTION, action);
        }

      if (elt)
        mpui_add_element (allmenuitem, elt);
      free (parser);
    }

  return allmenuitem;
}

mpui_menuitem_t *
mpui_parse_node_menu_item (mpui_t *mpui, char **attribs, char *body)
{
  mpui_menuitem_t *menuitem = NULL;
  ASX_Parser_t* parser;
  char *element;

  menuitem = mpui_menuitem_new ();
          
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
        {
          mpui_obj_t *obj = mpui_parse_node_obj (mpui, attribs);
          elt = mpui_element_new (MPUI_OBJ, obj);
        }
      else if (!strcmp (element, "img"))
        {
          mpui_img_t *img = mpui_parse_node_img (mpui, attribs);
          elt = mpui_element_new (MPUI_IMG, img);
        }
      else if (!strcmp (element, "str"))
        {
/*           mpui_str_t *str = mpui_parse_node_str (mpui, attribs); */
/*           elt = mpui_element_new (MPUI_STR, str); */
        }
      else if (!strcmp (element, "action"))
        {
          mpui_action_t *action = mpui_parse_node_action (attribs);
          elt = mpui_element_new (MPUI_ACTION, action);
        }
      
      if (elt)
        mpui_add_element (menuitem, elt);
      free (parser);
    }
  asx_free_attribs (attribs);

  return menuitem;
}

mpui_menu_t *
mpui_parse_node_menu (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *orientation, *font, *x, *y, *h, *w, *element;
  ASX_Parser_t* parser;
  mpui_menu_t *menu = NULL;
  mpui_menu_orientation_t morientation = MPUI_MENU_ORIENTATION_V;
  mpui_font_t *mfont = NULL;
  mpui_size_t mx, my, mh, mw;

  id = asx_get_attrib ("id", attribs);
  orientation = asx_get_attrib ("orientation", attribs);
  font = asx_get_attrib ("font", attribs);
  x = asx_get_attrib ("x", attribs);
  y = asx_get_attrib ("y", attribs);
  h = asx_get_attrib ("h", attribs);
  w = asx_get_attrib ("w", attribs);

/*   if (!strcmp (orientation, "horizontal")) */
/*     morientation = MPUI_MENU_ORIENTATION_H; */

  if (font)
    mfont = mpui_font_get (mpui, font);

  mx = mpui_parse_size (x);
  my = mpui_parse_size (y);
  mh = mpui_parse_size (h);
  mw = mpui_parse_size (w);

  if (id && mfont)
    menu = mpui_menu_new (id, morientation, mfont, mx, my, mh, mw);
  asx_free_attribs (attribs);

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
        {
          mpui_obj_t *obj = mpui_parse_node_obj (mpui, attribs);
          elt = mpui_element_new (MPUI_OBJ, obj);
        }
      else if (!strcmp (element, "img"))
        {
          mpui_img_t *img = mpui_parse_node_img (mpui, attribs);
          elt = mpui_element_new (MPUI_IMG, img);
        }
      else if (!strcmp (element, "str"))
        {
          mpui_str_t *str = mpui_parse_node_str (mpui, attribs);
          elt = mpui_element_new (MPUI_STR, str);
        }
      else if (!strcmp (element, "all-menu-items"))
        {
          mpui_allmenuitem_t *allmenuitem;
          allmenuitem = mpui_parse_node_menu_all_items (mpui, attribs, sbody);
          elt = mpui_element_new (MPUI_ALLMENUITEM, allmenuitem);
        }
      else if (!strcmp (element, "menu-item"))
        {
          mpui_menuitem_t *menuitem;
          menuitem = mpui_parse_node_menu_item (mpui, attribs, sbody);
          elt = mpui_element_new (MPUI_MENUITEM, menuitem);
        }

      if (elt)
        mpui_add_element (menu, elt);
      free (parser);
    }
  asx_free_attribs (attribs);
  
  return menu;
}

mpui_mnu_t *
mpui_parse_node_mnu (mpui_t *mpui, char **attribs)
{
  char *id;
  mpui_mnu_t *mnu = NULL;

  id = asx_get_attrib ("id", attribs);
  asx_free_attribs (attribs);

  if (id)
    {
      mpui_menu_t *menu = mpui_menu_get (mpui, id);
      if (menu)
        mnu = mpui_mnu_new (menu);
    }

  return mnu;
}

mpui_menus_t *
mpui_parse_node_menus (mpui_t *mpui, char **attribs, char *body)
{
  ASX_Parser_t* parser;
  char *element;
  mpui_menus_t *menus;

  menus = mpui_menus_new ();

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
          mpui_menus_add (menus, menu);
        }
      free (parser);
    }
  asx_free_attribs (attribs);

  return menus;
}

mpui_screen_t *
mpui_parse_node_screen (mpui_t *mpui, char **attribs, char *body)
{
  char *id, *element;
  ASX_Parser_t* parser;
  mpui_screen_t *screen = NULL;
  
  id = asx_get_attrib ("id", attribs);
  asx_free_attribs (attribs);

  if (id)
    screen = mpui_screen_new (id);

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
        {
          mpui_obj_t *obj = mpui_parse_node_obj (mpui, attribs);
          elt = mpui_element_new (MPUI_OBJ, obj);
        }
      else if (!strcmp (element, "img"))
        {
          mpui_img_t *img = mpui_parse_node_img (mpui, attribs);
          elt = mpui_element_new (MPUI_IMG, img);
        }
      else if (!strcmp (element, "str"))
        {
          mpui_str_t *str = mpui_parse_node_str (mpui, attribs);
          elt = mpui_element_new (MPUI_STR, str);
        }
      else if (!strcmp (element, "mnu"))
        {
          mpui_mnu_t *mnu = mpui_parse_node_mnu (mpui, attribs);
          elt = mpui_element_new (MPUI_MNU, mnu);
        }

      if (elt)
        mpui_add_element (screen, elt);
      free (parser);
    }
  asx_free_attribs (attribs);
  
  return screen;
}

mpui_screens_t *
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

  /*   screens->menu = mpui_screen_get (mpui, menu); */
  /*   screens->ctrl = mpui_screen_get (mpui, control); */

  return screens;
}

mpui_t *
mpui_parse_config (char *buffer, int width, int height)
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
  mpui->width = width;
  mpui->height = height;

  while(1)
    {
      char *sbody;
      int r;

      r = mpui_parse_get_element (parser, &body, &element, &sbody, &attribs);
      if (r < 0)
        return NULL;
      else if (r == 0)
        return mpui;

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
        {
          mpui_fonts_t *fonts = mpui_parse_node_fonts (mpui, attribs, sbody);
          mpui_fonts_add (mpui, fonts);
        }
      else if (!strcmp (element, "objects"))
        {
          mpui_objects_t *objects;
          objects = mpui_parse_node_objects (mpui, attribs, sbody);
          mpui_objects_add (mpui, objects);
        }
      else if (!strcmp (element, "menus"))
        {
          mpui_menus_t *menus = mpui_parse_node_menus (mpui, attribs, sbody);
          mpui_menus_add (mpui, menus);
        }
      else if (!strcmp (element, "screens"))
        {
          mpui_screens_t *screens;
          screens = mpui_parse_node_screens (mpui, attribs, sbody);
          mpui->screens = screens;
        }

      free (element);
    }
  free (parser);
  return mpui;
}

mpui_t *
mpui_parse_config_file (char *filename, int width, int height)
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
  mpui = mpui_parse_config (buffer, width, height);
  free (buffer);

  return mpui;
}
