/*
 *  mpui_cmd.c: libmpui actions execution.
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
#include "mpui_cmd.h"


typedef void (* mpui_cmd_action_t) (mpui_element_t *element, void *data);

static void
mpui_cmd_for_each_element (mpui_element_t **elements, char *element_id,
                           mpui_cmd_action_t func, void *data)
{
  for (; *elements; elements++)
    {
      if (!element_id
          || ((*elements)->id && !strcmp ((*elements)->id, element_id)))
        func (*elements, data);
      if ((*elements)->flags & MPUI_FLAG_CONTAINER)
        mpui_cmd_for_each_element (((mpui_container_t *) *elements)->elements,
                                   element_id, func, data);
    }
}


void
mpui_cmd_screen (mpui_t *mpui, char *screen_id)
{
  mpui_screen_t *screen = NULL;

  screen = mpui_screen_get (mpui->screens, screen_id);
  if (screen)
    {
      mpui->previous_screen = mpui->current_screen;
      mpui->current_screen = screen;
      mpui_focus_box_first (mpui->current_screen);
    }
}


void
mpui_cmd_popup (mpui_t *mpui, char *popup_id)
{
  mpui_popup_t *popup;

  popup = mpui_popup_get (mpui->popups, popup_id);
  mpui_popup_add (mpui->current_screen, popup);
}

void
mpui_cmd_popup_close (mpui_t *mpui)
{
  mpui_popup_remove (mpui->current_screen);
}


static void
mpui_cmd_hide_func (mpui_element_t *element, void *data)
{
  element->flags |= MPUI_FLAG_HIDDEN;
}

void
mpui_cmd_hide (mpui_t *mpui, char *element_id)
{
  mpui_cmd_for_each_element (mpui->current_screen->elements, element_id,
                             mpui_cmd_hide_func, NULL);
}


static void
mpui_cmd_show_func (mpui_element_t *element, void *data)
{
  element->flags &= ~MPUI_FLAG_HIDDEN;
}

void
mpui_cmd_show (mpui_t *mpui, char *element_id)
{
  mpui_cmd_for_each_element (mpui->current_screen->elements, element_id,
                             mpui_cmd_show_func, NULL);
}
