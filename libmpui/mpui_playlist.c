/*
 *  mpui_playlist.c: libmpui playlist management.
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

#include "mpui_struct.h"
#include "mpui_focus.h"
#include "mpui_parser.h"
#include "mpui_playlist.h"


static void
mpui_playlist_add_item (mpui_playlist_t *playlist, char *filename,
                        mpui_size_t *cx, mpui_size_t *cy, mpui_size_t *size)
{
  mpui_menuitem_t *menuitem;
  mpui_action_t *action;
  mpui_string_t *string;
  mpui_str_t *str;
  mpui_coord_t x, y, w, h;
  mpui_size_t offset;
  char *name, *command, *id = ((mpui_menu_t *) playlist)->id;

  name = (name=strrchr (filename, '/')) ? name+1 : filename;

  menuitem = mpui_menuitem_new ();
  ((mpui_element_t *) menuitem)->x.val = *cx;
  ((mpui_element_t *) menuitem)->y.val = *cy;

  x.val = 0;
  x.str = NULL;
  y.val = 0;
  y.str = NULL;
  w.str = NULL;
  h.str = NULL;

  string = mpui_string_new (NULL, name, MPUI_ENCODING_UTF8);
  str = mpui_str_new (string, x, y, 0, playlist->menu.font,
                      MPUI_FONT_SIZE_DEFAULT,
                      NULL, NULL, NULL, MPUI_DISPLAY_ALWAYS);

  if (x.val + ((mpui_element_t *) str)->w.val >= playlist->item_w)
    ((mpui_element_t *) str)->w.val = playlist->item_w - x.val;
  offset = playlist->item_w - ((mpui_element_t *)str)->w.val - x.val;
  switch (playlist->align)
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

  command = (char *) malloc (2*strlen (filename) + strlen (id) + 25);
  sprintf (command, "mpui_playlist_remove %s '", id);
  name = command + strlen (command);
  while (*filename)
    {
      /* escape ' and \ with \ */
      if (*filename == '\\' || *filename == '\'')
        *name++ = '\\';
      *name++ = *filename++;
    }
  *name++ = '\'';
  *name = '\0';
  action = mpui_action_new (command, MPUI_WHEN_VALIDATE);
  free (command);
  mpui_container_actions_add ((mpui_container_t *) menuitem, action);

  mpui_elements_get_size ((mpui_element_t *) menuitem,
                          ((mpui_container_t *) menuitem)->elements);
  ((mpui_element_t *)menuitem)->w.val = playlist->item_w;

  if (((mpui_menu_t *) playlist)->orientation == MPUI_ORIENTATION_H
      || (((mpui_menu_t *) playlist)->orientation & MPUI_ORIENTATION_H
          && ((mpui_menu_t *) playlist)->scrolling == MPUI_ORIENTATION_V))
    *cx += ((mpui_element_t *) menuitem)->w.val;
  else
    *cy += ((mpui_element_t *) menuitem)->h.val;

  mpui_menu_elements_add ((mpui_menu_t *) playlist,
                          (mpui_element_t *) menuitem);

  if (((mpui_menu_t *) playlist)->orientation == MPUI_ORIENTATION_H
      || (((mpui_menu_t *) playlist)->orientation & MPUI_ORIENTATION_H
          && ((mpui_menu_t *) playlist)->scrolling == MPUI_ORIENTATION_V))
    *size = ((mpui_element_t *) menuitem)->h.val;
  else
    *size = ((mpui_element_t *) menuitem)->w.val;
}

void
mpui_playlist_generate (mpui_playlist_t *playlist)
{
  mpui_size_t x, y, val, max = 0;
  char **item;

  if (playlist->border)
    mpui_menu_elements_add ((mpui_menu_t *) playlist,
                            (mpui_element_t *) playlist->border);
  if (playlist->item_border)
    mpui_menu_elements_add ((mpui_menu_t *) playlist,
                            (mpui_element_t *) playlist->item_border);

  x = y = playlist->spacing / 2;

  for (item=playlist->items; *item; item++)
    {
      mpui_playlist_add_item (playlist, *item, &x, &y, &val);

      if (val > max)
        max = val;

      if (((mpui_menu_t *) playlist)->orientation
          == (MPUI_ORIENTATION_H|MPUI_ORIENTATION_V))
        {
          if (((mpui_menu_t *) playlist)->scrolling == MPUI_ORIENTATION_V)
            {
              x += playlist->spacing;
              if (x >= ((mpui_menu_t *) playlist)->w)
                {
                  x = playlist->spacing / 2;
                  y += max + playlist->spacing;
                  max = 0;
                }
            }
          else
            {
              y += playlist->spacing;
              if (y >= ((mpui_menu_t *) playlist)->h)
                {
                  y = playlist->spacing / 2;
                  x += max + playlist->spacing;
                  max = 0;
                }
            }
        }
      else if (((mpui_menu_t *) playlist)->orientation == MPUI_ORIENTATION_V)
        y += playlist->spacing;
      else
        x += playlist->spacing;
    }
}

static void
mpui_playlist_clean (mpui_playlist_t *playlist)
{
  mpui_element_t **e;

  for (e = ((mpui_menu_t *) playlist)->elements; *e; e++)
    if (*e != (mpui_element_t *) playlist->border
        && *e != (mpui_element_t *) playlist->item_border)
      mpui_element_free (*e);
  *((mpui_menu_t *) playlist)->elements = NULL;
}


void
mpui_playlist_remove (mpui_playlist_t *playlist, char *filename)
{
  char **item;

  for (item=playlist->items; *item; item++)
    if (!strcmp (*item, filename))
      {
        free (*item);
        do {
          *item = *(item+1);
        } while (*item++);
        playlist->need_generate = 1;
        break;
      }
}

void
mpui_playlist_add (mpui_playlist_t *playlist, char *filename)
{
  if (filename)
    {
      playlist->items = mpui_list_add (playlist->items, strdup (filename));
      playlist->need_generate = 1;
    }
}

void
mpui_playlist_empty (mpui_playlist_t *playlist)
{
  char **item;

  for (item=playlist->items; *item; item++)
    free (*item);
  *playlist->items = NULL;
  playlist->need_generate = 1;
}

void
mpui_playlist_load (mpui_playlist_t *playlist, char *filename)
{
  char line[NAME_MAX], buff[NAME_MAX], *name;
  FILE *fp;
  int l, len;

  mpui_playlist_empty (playlist);

  strncpy (buff, filename, sizeof (buff));
  buff[NAME_MAX-1] = '\0';
  name = strrchr (buff, '/');
  if (!name++)
    name = buff;
  len = buff+NAME_MAX - name - 1;

  if (!(fp = fopen (filename, "r")))
    return;

  while (fgets (line, sizeof (line), fp))
    if (*line != '#')
      {
        l = strlen (line) - 1;
        if (line[l] == '\n')
          line[l--] = '\0';
        if (line[l] == '\r')
          line[l] = '\0';
        strncpy (name, line, len);
        mpui_playlist_add (playlist, buff);
      }

  fclose (fp);
}

void
mpui_playlist_update (mpui_mnu_t *mnu)
{
  mpui_playlist_t *playlist = (mpui_playlist_t *) mnu->menu;

  if (playlist->need_generate)
    {
      mpui_focus_unfocus ((mpui_focus_box_t *) mnu);
      mpui_playlist_clean (playlist);
      mpui_playlist_generate (playlist);
      ((mpui_container_t *) mnu)->elements = mnu->menu->elements;
      mpui_focus_first ((mpui_focus_box_t *) mnu);
      playlist->need_generate = 0;
    }
}
