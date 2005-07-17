/*
 *  mpui_focus.c: libmpui focus manager.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mpui_struct.h"
#include "mpui_focus.h"


static int
mpui_focus_box_change (mpui_screen_t *screen, mpui_element_t **element)
{
  mpui_container_t *container;
  mpui_element_t **e;
  int keymap = 0;

  for (e=((mpui_container_t *) *element)->elements; *e; e++)
    if ((*e)->flags & MPUI_FLAG_FOCUSABLE)
      break;
  if (!*e)
    return 0;

  if (screen->focus_box > screen->elements)
    keymap = 1;
  if (keymap && (container = (mpui_container_t *) *screen->focus_box)
      && container->keymaps)
    mp_input_remove_binds_filter(container->keymaps->binds);
  screen->focus_box = element;
  container = (mpui_container_t *) *element;
  if (keymap && container->keymaps)
    mp_input_add_binds_filter(container->keymaps->binds);
  return 1;
}

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
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX
        && !((*elements)->flags & MPUI_FLAG_HIDDEN))
      if (mpui_focus_box_change (screen, elements))
        return;
  for (elements=screen->elements; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX
        && !((*elements)->flags & MPUI_FLAG_HIDDEN))
      if (mpui_focus_box_change (screen, elements))
        return;
}

void
mpui_focus_box_previous (mpui_screen_t *screen)
{
  mpui_element_t **elements;

  for (elements=screen->focus_box-1; elements >= screen->elements; elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX
        && !((*elements)->flags & MPUI_FLAG_HIDDEN))
      if (mpui_focus_box_change (screen, elements))
        return;
  for (elements=screen->focus_box+1; *elements; elements++);
  for (elements--; elements > screen->focus_box; elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX
        && !((*elements)->flags & MPUI_FLAG_HIDDEN))
      if (mpui_focus_box_change (screen, elements))
        return;
}


static void
mpui_focus_change (mpui_focus_box_t *focus_box, mpui_element_t **element)
{
  if (focus_box->focus >= focus_box->container.elements)
    mpui_focus_action_exec (focus_box, MPUI_WHEN_UNFOCUS);
  focus_box->focus = element;
  if (focus_box->focus)
    mpui_focus_action_exec (focus_box, MPUI_WHEN_FOCUS);
}

int
mpui_focus_first (mpui_focus_box_t *focus_box)
{
  focus_box->focus = focus_box->container.elements - 1;
  mpui_focus_next (focus_box);
  focus_box->xoffset = 0;
  focus_box->yoffset = 0;
  if (focus_box->focus >= focus_box->container.elements)
    return 1;
  focus_box->focus = NULL;
  return 0;
}

void
mpui_focus_next (mpui_focus_box_t *focus_box)
{
  mpui_element_t **elements;

  if (!focus_box->focus)
    return;

  for (elements=focus_box->focus+1; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      {
        if ((*elements)->type == MPUI_MENUITEM)
          {
            mpui_size_t offset;
            if (focus_box->scrolling & MPUI_ORIENTATION_V)
              {
                offset = (*elements)->y.val + (*elements)->h.val
                         - focus_box->yoffset
                         - ((mpui_element_t *) focus_box)->h.val;
                if (offset > 0)
                  focus_box->yoffset += offset;
              }
            if (focus_box->scrolling & MPUI_ORIENTATION_H)
              {
                offset = (*elements)->x.val + (*elements)->w.val
                         - focus_box->xoffset
                         - ((mpui_element_t *) focus_box)->w.val;
                if (offset > 0)
                  focus_box->xoffset += offset;
              }
          }
        mpui_focus_change (focus_box, elements);
        return;
      }
  for (elements=focus_box->container.elements; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      {
        focus_box->xoffset = 0;
        focus_box->yoffset = 0;
        mpui_focus_change (focus_box, elements);
        return;
      }
}

void
mpui_focus_previous (mpui_focus_box_t *focus_box)
{
  mpui_element_t **elements;

  if (!focus_box->focus)
    return;

  for (elements=focus_box->focus-1; elements>=focus_box->container.elements;
       elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      {
        if ((*elements)->type == MPUI_MENUITEM)
          {
            mpui_size_t offset;
            if (focus_box->scrolling & MPUI_ORIENTATION_V)
              {
                offset = focus_box->yoffset - (*elements)->y.val;
                if (offset > 0)
                  focus_box->yoffset -= offset;
              }
            if (focus_box->scrolling & MPUI_ORIENTATION_H)
              {
                offset = focus_box->xoffset - (*elements)->x.val;
                if (offset > 0)
                  focus_box->xoffset -= offset;
              }
          }
        mpui_focus_change (focus_box, elements);
        return;
      }
  for (elements=focus_box->focus+1; *elements; elements++);
  for (elements--; elements > focus_box->focus; elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      {
        if ((*elements)->type == MPUI_MENUITEM)
          {
            mpui_size_t offset;
            if (focus_box->scrolling & MPUI_ORIENTATION_V)
              {
                offset = (*elements)->y.val + (*elements)->h.val
                         - focus_box->yoffset
                         - ((mpui_element_t *) focus_box)->h.val;
                if (offset > 0)
                  focus_box->yoffset += offset;
              }
            if (focus_box->scrolling & MPUI_ORIENTATION_H)
              {
                offset = (*elements)->x.val + (*elements)->w.val
                         - focus_box->xoffset
                         - ((mpui_element_t *) focus_box)->w.val;
                if (offset > 0)
                  focus_box->xoffset += offset;
              }
          }
        mpui_focus_change (focus_box, elements);
        return;
      }
}

void
mpui_focus_next_line (mpui_focus_box_t *focus_box)
{
  mpui_element_t **elements;

  if (!focus_box->focus)
    return;

  for (elements=focus_box->focus+1; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE
        && ((focus_box->scrolling == MPUI_ORIENTATION_V
             && (*elements)->y.val > (*focus_box->focus)->y.val + 3
             && (*elements)->x.val >= (*focus_box->focus)->x.val - 3)
            || (focus_box->scrolling == MPUI_ORIENTATION_H
                && (*elements)->x.val > (*focus_box->focus)->x.val + 3
                && (*elements)->y.val >= (*focus_box->focus)->y.val - 3)))
      {
        if ((*elements)->type == MPUI_MENUITEM)
          {
            mpui_size_t offset;
            if (focus_box->scrolling & MPUI_ORIENTATION_V)
              {
                offset = (*elements)->y.val + (*elements)->h.val
                         - focus_box->yoffset
                         - ((mpui_element_t *) focus_box)->h.val;
                if (offset > 0)
                  focus_box->yoffset += offset;
              }
            if (focus_box->scrolling & MPUI_ORIENTATION_H)
              {
                offset = (*elements)->x.val + (*elements)->w.val
                         - focus_box->xoffset
                         - ((mpui_element_t *) focus_box)->w.val;
                if (offset > 0)
                  focus_box->xoffset += offset;
              }
          }
        mpui_focus_change (focus_box, elements);
        return;
      }
  for (elements=focus_box->container.elements; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE
        && ((focus_box->scrolling == MPUI_ORIENTATION_V
             && (*elements)->x.val >= (*focus_box->focus)->x.val - 3)
            || (focus_box->scrolling == MPUI_ORIENTATION_H
                && (*elements)->y.val >= (*focus_box->focus)->y.val - 3)))
      {
        focus_box->xoffset = 0;
        focus_box->yoffset = 0;
        mpui_focus_change (focus_box, elements);
        return;
      }
}

void
mpui_focus_previous_line (mpui_focus_box_t *focus_box)
{
  mpui_element_t **elements;

  if (!focus_box->focus)
    return;

  for (elements=focus_box->focus-1; elements>=focus_box->container.elements;
       elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE
        && ((focus_box->scrolling == MPUI_ORIENTATION_V
             && (*elements)->y.val < (*focus_box->focus)->y.val - 3
             && (*elements)->x.val <= (*focus_box->focus)->x.val + 3)
            || (focus_box->scrolling == MPUI_ORIENTATION_H
                && (*elements)->x.val < (*focus_box->focus)->x.val - 3
                && (*elements)->y.val <= (*focus_box->focus)->y.val + 3)))
      {
        if ((*elements)->type == MPUI_MENUITEM)
          {
            mpui_size_t offset;
            if (focus_box->scrolling & MPUI_ORIENTATION_V)
              {
                offset = focus_box->yoffset - (*elements)->y.val;
                if (offset > 0)
                  focus_box->yoffset -= offset;
              }
            if (focus_box->scrolling & MPUI_ORIENTATION_H)
              {
                offset = focus_box->xoffset - (*elements)->x.val;
                if (offset > 0)
                  focus_box->xoffset -= offset;
              }
          }
        mpui_focus_change (focus_box, elements);
        return;
      }
  for (elements=focus_box->focus+1; *elements; elements++);
  for (elements--; elements > focus_box->focus; elements--)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE
        && ((focus_box->scrolling == MPUI_ORIENTATION_V
             && (*elements)->x.val >= (*focus_box->focus)->x.val - 3
             && (*elements)->x.val <= (*focus_box->focus)->x.val + 3)
            || (focus_box->scrolling == MPUI_ORIENTATION_H
                && (*elements)->y.val >= (*focus_box->focus)->y.val - 3
                && (*elements)->y.val <= (*focus_box->focus)->y.val + 3)))
      {
        if ((*elements)->type == MPUI_MENUITEM)
          {
            mpui_size_t offset;
            if (focus_box->scrolling & MPUI_ORIENTATION_V)
              {
                offset = (*elements)->y.val + (*elements)->h.val
                         - focus_box->yoffset
                         - ((mpui_element_t *) focus_box)->h.val;
                if (offset > 0)
                  focus_box->yoffset += offset;
              }
            if (focus_box->scrolling & MPUI_ORIENTATION_H)
              {
                offset = (*elements)->x.val + (*elements)->w.val
                         - focus_box->xoffset
                         - ((mpui_element_t *) focus_box)->w.val;
                if (offset > 0)
                  focus_box->xoffset += offset;
              }
          }
        mpui_focus_change (focus_box, elements);
        return;
      }
}

void
mpui_focus_element (mpui_focus_box_t *focus_box, mpui_element_t *element)
{
  mpui_element_t **elements;

  for (elements=focus_box->container.elements; *elements; elements++)
    if (*elements == element)
      {
        if (element->type == MPUI_MENUITEM)
          {
            mpui_size_t offset;
            if (focus_box->scrolling & MPUI_ORIENTATION_V)
              {
                offset = element->y.val + element->h.val - focus_box->yoffset
                         - ((mpui_element_t *) focus_box)->h.val;
                if (offset > 0)
                  focus_box->yoffset += offset;
              }
            if (focus_box->scrolling & MPUI_ORIENTATION_H)
              {
                offset = element->x.val + element->w.val - focus_box->xoffset
                         - ((mpui_element_t *) focus_box)->w.val;
                if (offset > 0)
                  focus_box->xoffset += offset;
              }
          }
        mpui_focus_change (focus_box, elements);
        return;
      }
}

void
mpui_focus_index (mpui_focus_box_t *focus_box, int index)
{
  mpui_element_t **elements;

  for (elements=focus_box->container.elements;
       *elements && elements[1] && index; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUSABLE)
      index--;

  mpui_focus_element (focus_box, *elements);
}

int
mpui_focus_get_index (mpui_focus_box_t *focus_box)
{
  mpui_element_t **elements;
  int index = 0;

  if (!focus_box->focus)
    return -1;

  for (elements=focus_box->container.elements; *elements; elements++, index++)
    if (elements == focus_box->focus)
      return index;

  return 0;
}

void
mpui_focus_unfocus (mpui_focus_box_t *focus_box)
{
  mpui_focus_change (focus_box, NULL);
}


void
mpui_focus_action_exec (mpui_focus_box_t *focus_box, mpui_action_when_t when)
{
  mpui_action_t **actions;

  if (!focus_box->focus)
    return;

  actions = ((mpui_container_t *) *focus_box->focus)->actions;

  if (actions)
    for (; *actions; actions++)
      if ((*actions)->when & when)
        {
          char *cmd = strdup ((*actions)->cmd);
          mp_input_queue_cmd (mp_input_parse_cmd (cmd));
          free (cmd);
        }

  return;
}


int
mpui_is_focused (mpui_screen_t *screen, mpui_element_t *element)
{
  mpui_element_t **elements;

  for (elements=screen->elements; *elements; elements++)
    if ((*elements)->flags & MPUI_FLAG_FOCUS_BOX
        && ((mpui_focus_box_t *) *elements)->focus
        && ((mpui_focus_box_t *) *elements)->focus[0] == element)
        return 1;
  return 0;
}

int
mpui_is_really_focused (mpui_screen_t *screen, mpui_element_t *element)
{
  if ((mpui_focus_box_t *) screen->focus_box
      && ((mpui_focus_box_t *) screen->focus_box[0])->focus)
    return ((mpui_focus_box_t *) screen->focus_box[0])->focus[0] == element;
  return 0;
}
