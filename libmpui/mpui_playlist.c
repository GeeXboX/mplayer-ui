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

#include "playtree.h"

#include "mpui_struct.h"
#include "mpui_focus.h"
#include "mpui_parser.h"
#include "mpui_playlist.h"

static void
mpui_playlist_force_refresh (mpui_t *mpui)
{
  mpui_element_t **elements;

  if (mpui->current_screen)
    for (elements = mpui->current_screen->elements; *elements; elements++)
      if ((*elements)->type == MPUI_MNU
          && ((mpui_mnu_t *) *elements)->menu->is_playlist)
        ((mpui_mnu_t *) *elements)->menu->need_refresh = 1;
}

void
mpui_playlist_remove_entry (mpui_t *mpui, char *filename)
{
  extern play_tree_iter_t* playtree_iter;
  play_tree_t *pt;

  for (pt = playtree_iter->tree ; pt->prev != NULL ; pt = pt->prev)
    /* NOP */;

  for (; pt != NULL ; pt = pt->next )
    {
      char *tmp;

      if (!pt->files)
        continue;

      tmp = strrchr (pt->files[0], '/');
      if (!tmp)
        continue;

      if (!strcmp (tmp + 1, filename))
        {
          play_tree_free (pt, 1);
          mpui_playlist_force_refresh (mpui);
          break;
        }
    }
}

void
mpui_playlist_add_entry (mpui_t *mpui, char *filename)
{
  extern play_tree_iter_t* playtree_iter;

  pt_iter_add_file (playtree_iter, filename);
  mpui_playlist_force_refresh (mpui);
}

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
  char *command;

  menuitem = mpui_menuitem_new ();
  ((mpui_element_t *) menuitem)->x.val = *cx;
  ((mpui_element_t *) menuitem)->y.val = *cy;

  x.val = 0;
  x.str = NULL;
  y.val = 0;
  y.str = NULL;
  w.str = NULL;
  h.str = NULL;

  string = mpui_string_new (NULL, filename, MPUI_ENCODING_UTF8);
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

  command = (char *) malloc (strlen (filename) + 24);
  sprintf (command, "mpui_playlist_remove '%s'", filename);
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
  extern play_tree_iter_t* playtree_iter;
  mpui_size_t x, y, val, max = 0;
  play_tree_t* pt;

  if (playlist->border)
    mpui_menu_elements_add ((mpui_menu_t *) playlist,
                            (mpui_element_t *) playlist->border);
  if (playlist->item_border)
    mpui_menu_elements_add ((mpui_menu_t *) playlist,
                            (mpui_element_t *) playlist->item_border);

  x = y = playlist->spacing / 2;

  for (pt = playtree_iter->tree ; pt->prev != NULL ; pt = pt->prev)
    /* NOP */;

  for (; pt != NULL ; pt = pt->next )
    {
      char *tmp;

      if (!pt->files)
        continue;

      tmp = strrchr (pt->files[0], '/');
      if (!tmp)
        continue;
      
      mpui_playlist_add_item (playlist, tmp + 1, &x, &y, &val);

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
mpui_playlist_update (mpui_t *mpui, mpui_mnu_t *mnu)
{
  mpui_playlist_t *playlist = (mpui_playlist_t *) mnu->menu;

  if (mnu->menu->need_refresh)
    {
      mpui_focus_unfocus ((mpui_focus_box_t *) mnu);
      mpui_playlist_clean (playlist);
      mpui_playlist_generate (playlist);
      ((mpui_container_t *) mnu)->elements = mnu->menu->elements;
      mpui_focus_first ((mpui_focus_box_t *) mnu);
      mnu->menu->need_refresh = 0;
    }
}
