/*
 *  mpui_focus.c: libmpui focus manager.
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

#include "mpui_struct.h"
#include "mpui_focus.h"


int
mpui_focus_box_first (mpui_screen_t *screen)
{
  screen->focus_box = screen->elements - 1;
  mpui_focus_box_next (screen);
  if (screen->focus_box >= screen->elements)
    return 1;
  screen->focus_box = NULL;
  return 0;
}

void
mpui_focus_box_next (mpui_screen_t *screen)
{
  mpui_element_t **elements;

  for (elements=screen->focus_box+1; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX)
      {
        screen->focus_box = elements;
        return;
      }
  for (elements=screen->elements; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX)
      {
        screen->focus_box = elements;
        return;
      }
}

void
mpui_focus_box_previous (mpui_screen_t *screen)
{
  mpui_element_t **elements;

  for (elements=screen->focus_box-1; elements >= screen->elements; elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX)
      {
        screen->focus_box = elements;
        return;
      }
  for (elements=screen->focus_box+1; *elements; elements++);
  for (elements--; elements > screen->focus_box; elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX)
      {
        screen->focus_box = elements;
        return;
      }
}


int
mpui_focus_first (mpui_focus_box_t *focus_box)
{
  focus_box->focus = focus_box->elements - 1;
  mpui_focus_next (focus_box);
  if (focus_box->focus >= focus_box->elements)
    return 1;
  focus_box->focus = NULL;
  return 0;
}

void
mpui_focus_next (mpui_focus_box_t *focus_box)
{
  mpui_element_t **elements;

  for (elements=focus_box->focus+1; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      {
        focus_box->focus = elements;
        return;
      }
  for (elements=focus_box->elements; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      {
        focus_box->focus = elements;
        return;
      }
}

void
mpui_focus_previous (mpui_focus_box_t *focus_box)
{
  mpui_element_t **elements;

  for (elements=focus_box->focus-1; elements>=focus_box->elements; elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      {
        focus_box->focus = elements;
        return;
      }
  for (elements=focus_box->focus+1; *elements; elements++);
  for (elements--; elements > focus_box->focus; elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      {
        focus_box->focus = elements;
        return;
      }
}


int
mpui_is_focused (mpui_screen_t *screen, mpui_element_t *element)
{
  mpui_element_t **elements;

  for (elements=screen->elements; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX)
      if (((mpui_focus_box_t *) *elements)->focus[0] == element)
        return 1;
  return 0;
}

int
mpui_is_really_focused (mpui_screen_t *screen, mpui_element_t *element)
{
  return ((mpui_focus_box_t *) screen->focus_box[0])->focus[0] == element;
}