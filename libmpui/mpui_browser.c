/*
 *  mpui_browser.c: libmpui file browser management.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "mpui_struct.h"
#include "mpui_focus.h"
#include "mpui_parser.h"
#include "mpui_browser.h"


static mpui_filetype_t *
mpui_browser_get_filetype (mpui_browser_t *browser, char *filename)
{
  mpui_filetype_t **filetypes = browser->filter->filetypes;
  struct stat st;
  char **exts;
  int len, l;

  stat (filename, &st);
  len = strlen (filename);

  for (filetypes=browser->filter->filetypes; *filetypes; filetypes++)
    switch ((*filetypes)->match)
      {
        case MPUI_MATCH_DIR:
          if (S_ISDIR(st.st_mode))
            return *filetypes;
          break;
        case MPUI_MATCH_EXT:
          for (exts=(*filetypes)->exts; *exts; exts++)
            {
              l = strlen (*exts) + 1;
              if (len >= l && filename[len-l] == '.'
                  && !strcasecmp (*exts, filename+len-l+1))
                return *filetypes;
            }
          break;
        case MPUI_MATCH_ALL:
          return *filetypes;
          break;
      }

  return NULL;
}

static mpui_element_t *
mpui_browser_add_item (mpui_t *mpui, mpui_browser_t *browser, char *filename,
                       mpui_size_t *cx, mpui_size_t *cy, mpui_size_t *size)
{
  char name[NAME_MAX], cmd[NAME_MAX+32];
  mpui_filetype_t *filetype;
  mpui_menuitem_t *menuitem;
  mpui_string_t *string;
  mpui_str_t *str;
  mpui_image_t *icon = NULL;
  mpui_img_t *img;
  mpui_action_t **actions, *action;
  mpui_coord_t x, y, w, h;
  mpui_size_t offset;

  if (!strcmp (mpui->cwd, "/"))
    snprintf (name, sizeof (name), "/%s", filename);
  else
    snprintf (name, sizeof (name), "%s/%s", mpui->cwd, filename);

  filetype = mpui_browser_get_filetype (browser, name);
  if (!filetype)
    return NULL;
  icon = filetype->icon;

  menuitem = mpui_menuitem_new ();
  ((mpui_element_t *) menuitem)->x.val = *cx;
  ((mpui_element_t *) menuitem)->y.val = *cy;

  x.val = 0;
  x.str = NULL;
  y.val = 0;
  y.str = NULL;
  w.str = NULL;
  h.str = NULL;

  if (icon)
    {
      if (((mpui_menu_t *) browser)->orientation == MPUI_ORIENTATION_H
          || (((mpui_menu_t *) browser)->orientation & MPUI_ORIENTATION_H
              && ((mpui_menu_t *) browser)->scrolling == MPUI_ORIENTATION_V))
        y.val = icon->h + browser->spacing/2;
      else
        x.val = icon->w + browser->spacing/2;
    }

  string = mpui_string_new (NULL, filename, MPUI_ENCODING_UTF8);
  str = mpui_str_new (string, x, y, 0, browser->menu.font,
                      MPUI_FONT_SIZE_DEFAULT,
                      NULL, NULL, NULL, MPUI_DISPLAY_ALWAYS);
  if (x.val + ((mpui_element_t *) str)->w.val >= browser->item_w)
    ((mpui_element_t *) str)->w.val = browser->item_w - x.val;
  offset = browser->item_w - ((mpui_element_t *)str)->w.val - x.val;
  switch (browser->align)
    {
    case MPUI_ALIGNMENT_CENTER:
      offset >>= 1;
    case MPUI_ALIGNMENT_RIGHT:
      ((mpui_element_t *) str)->x.val += offset;
      break;
    case MPUI_ALIGNMENT_LEFT:
      offset = 0;
    default:
      break;
    }
  mpui_container_elements_add ((mpui_container_t *) menuitem, 
                               (mpui_element_t *) str);

  if (icon)
    {
      x.val = 0;
      y.val = 0;
      w.val = icon->w;
      h.val = icon->h;

      if (((mpui_menu_t *) browser)->orientation == MPUI_ORIENTATION_H
          || (((mpui_menu_t *) browser)->orientation & MPUI_ORIENTATION_H
              && ((mpui_menu_t *) browser)->scrolling == MPUI_ORIENTATION_V))
        x.val = (browser->item_w - icon->w) / 2;
      else
        {
          x.val += offset;
          offset = ((mpui_element_t *) str)->h.val - icon->h;
          if (offset < 0)
            ((mpui_element_t *) str)->y.val -= offset / 2;
          else
            y.val = offset / 2;
        }
      img = mpui_img_new (icon, x, y, w, h, 0, MPUI_DISPLAY_ALWAYS);
      mpui_img_load (mpui, img);
      mpui_clip (mpui, (mpui_element_t *) img, 0, 0, 0);
      mpui_container_elements_add ((mpui_container_t *) menuitem, 
                                   (mpui_element_t *) img);
    }

  if (filetype->match == MPUI_MATCH_DIR)
    {
      DIR *dir;
      if ((dir = opendir (name)))
        {
          closedir (dir);
          if (!strcmp (filename, ".."))
            {
              char *p;
              snprintf (name, sizeof (name), "%s", mpui->cwd);
              if ((p = strrchr (name, '/')))
                {
                  if (p == name)
                    p++;
                  *p = '\0';
                }
            }
        }
      else
        filetype = NULL;
    }
  if (filetype)
    {
      /* escape ' and \ with \ */
      char *s = cmd, *d = name;
      strcpy (cmd, name);
      while (*s)
        {
          if (*s == '\\' || *s == '\'')
            *d++ = '\\';
          *d++ = *s++;
        }
      *d = '\0';
      
      for (actions=filetype->actions; *actions; actions++)
        {
          snprintf (cmd, sizeof (cmd), (*actions)->cmd, name);
          action = mpui_action_new (cmd, (*actions)->when);
          mpui_container_actions_add ((mpui_container_t *) menuitem, action);
        }
    }

  mpui_elements_get_size ((mpui_element_t *) menuitem,
                          ((mpui_container_t *) menuitem)->elements);
  ((mpui_element_t *)menuitem)->w.val = browser->item_w;

  if (((mpui_menu_t *) browser)->orientation == MPUI_ORIENTATION_H
      || (((mpui_menu_t *) browser)->orientation & MPUI_ORIENTATION_H
          && ((mpui_menu_t *) browser)->scrolling == MPUI_ORIENTATION_V))
    *cx += ((mpui_element_t *) menuitem)->w.val;
  else
    *cy += ((mpui_element_t *) menuitem)->h.val;

  mpui_menu_elements_add ((mpui_menu_t *) browser, (mpui_element_t *) menuitem);

  if (((mpui_menu_t *) browser)->orientation == MPUI_ORIENTATION_H
      || (((mpui_menu_t *) browser)->orientation & MPUI_ORIENTATION_H
          && ((mpui_menu_t *) browser)->scrolling == MPUI_ORIENTATION_V))
    *size = ((mpui_element_t *) menuitem)->h.val;
  else
    *size = ((mpui_element_t *) menuitem)->w.val;

  return (mpui_element_t *) menuitem;
}


mpui_element_t *
mpui_browser_generate (mpui_t *mpui, mpui_browser_t *browser)
{
  mpui_element_t *element, *select = NULL;
  struct dirent **dirent;
  mpui_size_t x, y, val, max = 0;
  int i, n;

  if (browser->border)
    mpui_menu_elements_add ((mpui_menu_t *) browser,
                            (mpui_element_t *) browser->border);
  if (browser->item_border)
    mpui_menu_elements_add ((mpui_menu_t *) browser, 
                            (mpui_element_t *) browser->item_border);

  x = y = browser->spacing/2;

  n = scandir (mpui->cwd, &dirent, NULL, alphasort);
  for (i=0; i<n; i++)
    {
      if (!strcmp (dirent[i]->d_name, ".")
          || (!strcmp (dirent[i]->d_name, "..") && !strcmp (mpui->cwd, "/")))
        {
          free (dirent[i]);
          continue;
        }
      element = mpui_browser_add_item (mpui, browser, dirent[i]->d_name,
                                       &x, &y, &val);
      if (mpui->lwd && !strcmp (mpui->lwd, dirent[i]->d_name))
        select = element;
      free (dirent[i]);
      if (!element)
        continue;
      if (val > max)
        max = val;
      if (((mpui_menu_t *) browser)->orientation
          == (MPUI_ORIENTATION_H|MPUI_ORIENTATION_V))
        {
          if (((mpui_menu_t *) browser)->scrolling == MPUI_ORIENTATION_V)
            {
              x += browser->spacing;
              if (x >= ((mpui_menu_t *) browser)->w)
                {
                  x = browser->spacing/2;
                  y += max + browser->spacing;
                  max = 0;
                }
            }
          else
            {
              y += browser->spacing;
              if (y >= ((mpui_menu_t *) browser)->h)
                {
                  y = browser->spacing/2;
                  x += max + browser->spacing;
                  max = 0;
                }
            }
        }
      else if (((mpui_menu_t *) browser)->orientation == MPUI_ORIENTATION_V)
        y += browser->spacing;
      else
        x += browser->spacing;
    }
  free (dirent);

  browser->cwd_id = mpui->cwd_id;
  return select;
}

void
mpui_browser_clean (mpui_browser_t *browser)
{
  mpui_element_t **e;

  for (e=((mpui_menu_t *) browser)->elements; *e; e++)
    if (*e != (mpui_element_t *) browser->border
        && *e != (mpui_element_t *) browser->item_border)
      mpui_element_free (*e);
  *((mpui_menu_t *) browser)->elements = NULL;
}

void
mpui_browser_update (mpui_t *mpui, mpui_mnu_t *mnu)
{
  mpui_browser_t *browser = (mpui_browser_t *) mnu->menu;
  mpui_element_t *element;

  if (browser->cwd_id != mpui->cwd_id)
    {
      mpui_focus_unfocus ((mpui_focus_box_t *) mnu);
      mpui_browser_clean (browser);
      element = mpui_browser_generate (mpui, browser);
      ((mpui_container_t *) mnu)->elements = mnu->menu->elements;
      if (element)
        mpui_focus_element ((mpui_focus_box_t *) mnu, element);
      else
        mpui_focus_first ((mpui_focus_box_t *) mnu);
    }
}

void
mpui_browser_cd (mpui_t *mpui, char *directory)
{
  int l = strlen (directory), i = directory[l-1] == '/' ? 0 : 1;

  if (!strncmp (directory, mpui->cwd, l) && !strchr (mpui->cwd + l + i, '/'))
    {
      if (l == 1)
        memmove (mpui->cwd+2, mpui->cwd+1, strlen (mpui->cwd));
      mpui->cwd[l] = '\0';
      mpui->lwd = mpui->cwd + l + 1;
    }
  else
    {
      strncpy (mpui->cwd, directory, sizeof (mpui->cwd)-1);
      mpui->lwd = NULL;
    }
  mpui->cwd_id++;
}
