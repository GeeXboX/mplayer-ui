/*
 *  mpui_tv.h: libmpui tv and dvb properties.
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
#include "mpui_tv.h"

void
mpui_tv_analog_channels_generate (mpui_t *mpui, mpui_menu_t *menu,
                                  mpui_size_t *mx, mpui_size_t *my,
                                  mpui_size_t *mw, mpui_size_t *mh,
                                  mpui_coord_t *spacing)
{
  extern char **tv_param_channels;
  char **channels;
  int idx = 1;

  if (!tv_param_channels)
    return;

  channels = tv_param_channels;
  while (*channels)
    {
      char *tmp, *sep, cmd[32];
      mpui_coord_t x = {0, NULL}, y = {0, NULL};
      mpui_menuitem_t *menuitem = NULL;
      mpui_container_t *container;
      mpui_action_t *action;
      mpui_string_t *string;
      mpui_element_t *elt;
      mpui_str_t *str;

      tmp = *(channels++);
      sep = strchr (tmp, '-');

      if (!sep)
        continue;

      menuitem = mpui_menuitem_new ();
      container = &menuitem->container;      

      string = mpui_string_new (NULL, sep + 1, MPUI_ENCODING_UTF8);
      str = mpui_str_new (string, x, y, 0, mpui->fonts->deflt,
                          MPUI_FONT_SIZE_DEFAULT, NULL, NULL,
                          NULL, MPUI_DISPLAY_ALWAYS);

      elt = (mpui_element_t *) str;
      elt->x.val = *mx;
      elt->y.val = *my;
      if (menu->orientation == MPUI_ORIENTATION_V)
        {
          *my += (*spacing).val + elt->h.val;
          if (elt->w.val > *mw)
            *mw = elt->w.val;
        }
      else
        {
          *mx += (*spacing).val + elt->w.val;
          if (elt->h.val > *mh)
            *mh = elt->h.val;
        }

      snprintf (cmd, 32, "loadfile tv://%d", idx++);
      action = mpui_action_new (cmd, MPUI_WHEN_VALIDATE);

      mpui_container_elements_add (container, (mpui_element_t *) str);
      mpui_container_actions_add (container, action);
      mpui_menu_elements_add (menu, (mpui_element_t *) menuitem);
    }
}
