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

#include <string.h>

#include "config.h"
#include "libmpdemux/dvbin.h"

#include "mpui_struct.h"
#include "mpui_tv.h"

static void
mpui_tv_add_channel (mpui_menu_t *menu, mpui_size_t *mx, mpui_size_t *my,
                     mpui_size_t *mw, mpui_size_t *mh, mpui_coord_t *spacing,
                     char *channel, char *command)
{
  mpui_coord_t x = {0, NULL}, y = {0, NULL};
  mpui_menuitem_t *menuitem = NULL;
  mpui_container_t *container;
  mpui_action_t *action;
  mpui_string_t *string;
  mpui_element_t *elt;
  mpui_str_t *str;

  menuitem = mpui_menuitem_new ();
  container = &menuitem->container;      

  string = mpui_string_new (NULL, channel, MPUI_ENCODING_UTF8);
  str = mpui_str_new (string, x, y, 0, menu->font, MPUI_FONT_SIZE_DEFAULT,
                      NULL, NULL, NULL, MPUI_DISPLAY_ALWAYS);

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

  mpui_container_elements_add (container, (mpui_element_t *) str);
  if (command)
    {
      action = mpui_action_new (command, MPUI_WHEN_VALIDATE);
      mpui_container_actions_add (container, action);
    }
  else
    ((mpui_element_t *) menuitem)->flags &= ~MPUI_FLAG_FOCUSABLE;
  mpui_menu_elements_add (menu, (mpui_element_t *) menuitem);
}

void
mpui_tv_analog_channels_generate (mpui_menu_t *menu,
                                  mpui_size_t *mx, mpui_size_t *my,
                                  mpui_size_t *mw, mpui_size_t *mh,
                                  mpui_coord_t *spacing,
                                  mpui_string_t *empty)
{
  extern char **tv_param_channels;
  char **channels;
  int idx = 1;

  if (!tv_param_channels)
    {
      mpui_tv_add_channel (menu, mx, my, mw, mh, spacing, empty->text, NULL);
      return;
    }

  channels = tv_param_channels;
  while (*channels)
    {
      char *tmp, *sep, cmd[32], channel[32], *chan=channel;

      tmp = *channels++;
      sep = strchr (tmp, '-');

      if (!sep)
        continue;
      while (*++sep && chan < (channel + sizeof (channel) - 1))
        *chan++ = *sep == '_' ? ' ' : *sep;
      *chan = '\0';

      snprintf (cmd, 32, "loadfile tv://%d", idx++);
      mpui_tv_add_channel (menu, mx, my, mw, mh, spacing, channel, cmd);
    }
}

void
mpui_tv_dvb_channels_generate (mpui_menu_t *menu,
                               mpui_size_t *mx, mpui_size_t *my,
                               mpui_size_t *mw, mpui_size_t *mh,
                               mpui_coord_t *spacing,
                               mpui_string_t *empty)
{
  dvb_config_t *dvb_config = NULL;
  int card;

  dvb_config = dvb_get_config();
  if (!dvb_config || !dvb_config->count)
    {
      mpui_tv_add_channel (menu, mx, my, mw, mh, spacing, empty->text, NULL);
      return;
    }

  for (card = 0; card < dvb_config->count; card++)
    {
      int chan;
      dvb_channels_list *channels_list;

      channels_list = dvb_config->cards[card].list;
      for (chan = 0; chan < channels_list->NUM_CHANNELS; chan++)
        {
          dvb_channel_t *channel;
          char *cmd;

          channel = &(channels_list->channels[chan]);
          cmd = malloc (25 + strlen (channel->name));
          sprintf (cmd, "loadfile 'dvb://%d@%s'", card, channel->name);
          mpui_tv_add_channel (menu, mx, my, mw, mh,
                               spacing, channel->name, cmd);
          free (cmd);
        }
    }

  free (dvb_config);
}
