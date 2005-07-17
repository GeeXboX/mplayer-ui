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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mpui_struct.h"
#include "mpui_focus.h"
#include "mpui_slideshow.h"
#include "mpui_playlist.h"
#include "mpui_cmd.h"
#include "mpui_info.h"


typedef void (* mpui_cmd_action_t) (mpui_t *mpui, mpui_element_t *element,
                                    void *data);

static void
mpui_cmd_for_each_element (mpui_t *mpui, mpui_element_t **elements,
                           char *element_id, mpui_type_t element_type,
                           mpui_flags_t flags,
                           mpui_cmd_action_t func, void *data)
{
  for (; *elements; elements++)
    {
      if ((!element_id
           || ((*elements)->id && !strcmp ((*elements)->id, element_id)))
          && (element_type == MPUI_ANY
              || element_type == (*elements)->type)
          && ((flags & (*elements)->flags) == flags))
        func (mpui, *elements, data);
      if ((*elements)->flags & MPUI_FLAG_CONTAINER)
        mpui_cmd_for_each_element (mpui,
                                   ((mpui_container_t *) *elements)->elements,
                                   element_id, element_type, flags, func,data);
    }
}


static void
mpui_cmd_screen_set_keymaps_func (mpui_t *mpui __attribute__((unused)),
                                  mpui_element_t *element, void *data)
{
  mpui_screen_t *screen = data;
  if ((element->flags & MPUI_FLAG_FOCUS_BOX) && *screen->focus_box != element)
    return;
  if (((mpui_container_t *) element)->keymaps)
    mp_input_add_binds_filter (((mpui_container_t *) element)->keymaps->binds);
}

void
mpui_cmd_screen_set_keymaps (mpui_t *mpui, mpui_screen_t *screen)
{
  if (screen->keymaps)
    mp_input_add_binds_filter (screen->keymaps->binds);
  mpui_cmd_for_each_element (mpui, screen->elements,
                             NULL, MPUI_ANY, MPUI_FLAG_CONTAINER,
                             mpui_cmd_screen_set_keymaps_func, screen);
}

static void
mpui_cmd_screen_unset_keymaps_func (mpui_t *mpui __attribute__((unused)),
                                    mpui_element_t *element, void *data)
{
  mpui_screen_t *screen = data;
  if ((element->flags & MPUI_FLAG_FOCUS_BOX) && *screen->focus_box != element)
    return;
  if (((mpui_container_t *) element)->keymaps)
    mp_input_remove_binds_filter(((mpui_container_t*)element)->keymaps->binds);
}

void
mpui_cmd_screen_unset_keymaps (mpui_t *mpui, mpui_screen_t *screen)
{
  if (screen->keymaps)
    mp_input_remove_binds_filter (screen->keymaps->binds);
  mpui_cmd_for_each_element (mpui, screen->elements,
                             NULL, MPUI_ANY, MPUI_FLAG_CONTAINER,
                             mpui_cmd_screen_unset_keymaps_func, screen);
}

void
mpui_cmd_screen (mpui_t *mpui, char *screen_id)
{
  mpui_screen_t *screen = mpui_screen_get (mpui->screens, screen_id);

  if (screen)
    {
      if (mpui->current_screen)
        mpui_cmd_screen_unset_keymaps (mpui, mpui->current_screen);
      mpui->current_screen = screen;
      mpui_focus_box_first (mpui->current_screen);
      mpui_cmd_screen_set_keymaps (mpui, screen);
    }
}


void
mpui_cmd_popup (mpui_t *mpui, char *popup_id)
{
  mpui_popup_t *popup;

  if (mpui->current_screen && (popup = mpui_popup_get(mpui->popups, popup_id)))
    {
      if (mpui_screen_has_popups (mpui->current_screen))
        {
          mpui_container_t *c;
          c = (mpui_container_t*) mpui_screen_last_popup(mpui->current_screen);
          if (c->keymaps)
            mp_input_remove_binds_filter (c->keymaps->binds);
        }
      else if (mpui->current_screen->keymaps)
        mpui_cmd_screen_unset_keymaps (mpui, mpui->current_screen);
      mpui_screen_popups_add (mpui->current_screen, popup);
      if (((mpui_container_t *) popup)->keymaps)
        mp_input_add_binds_filter (((mpui_container_t*)popup)->keymaps->binds);
    }
}

void
mpui_cmd_popup_close (mpui_t *mpui)
{
  if (mpui->current_screen)
    {
      mpui_popup_t *popup;
      popup = mpui_screen_popups_remove_last (mpui->current_screen);
      if (popup && ((mpui_container_t *) popup)->keymaps)
        mp_input_remove_binds_filter (((mpui_container_t *) popup)->keymaps->binds);
      if (mpui_screen_has_popups (mpui->current_screen))
        {
          mpui_container_t *c;
          c = (mpui_container_t*) mpui_screen_last_popup(mpui->current_screen);
          if (c->keymaps)
            mp_input_add_binds_filter (c->keymaps->binds);
        }
      else if (mpui->current_screen->keymaps)
        mpui_cmd_screen_set_keymaps (mpui, mpui->current_screen);
    }
}


static void
mpui_cmd_hide_func (mpui_t *mpui __attribute__((unused)),
                    mpui_element_t *element,
                    void *data __attribute__((unused)))
{
  element->flags |= MPUI_FLAG_HIDDEN;
}

void
mpui_cmd_hide (mpui_t *mpui, char *element_id)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               element_id, MPUI_ANY, 0,
                               mpui_cmd_hide_func, NULL);
}


static void
mpui_cmd_show_func (mpui_t *mpui __attribute__((unused)),
                    mpui_element_t *element,
                    void *data __attribute__((unused)))
{
  element->flags &= ~MPUI_FLAG_HIDDEN;
}

void
mpui_cmd_show (mpui_t *mpui, char *element_id)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               element_id, MPUI_ANY, 0,
                               mpui_cmd_show_func, NULL);
}


static void
mpui_cmd_hide_switch_func (mpui_t *mpui __attribute__((unused)),
                           mpui_element_t *element,
                           void *data __attribute__((unused)))
{
  if (element->flags & MPUI_FLAG_HIDDEN)
    element->flags &= ~MPUI_FLAG_HIDDEN;
  else
    element->flags |= MPUI_FLAG_HIDDEN;
}

void
mpui_cmd_hide_switch (mpui_t *mpui, char *element_id)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               element_id, MPUI_ANY, 0,
                               mpui_cmd_hide_switch_func, NULL);
}


void
mpui_cmd_focus_action_exec (mpui_t *mpui)
{
  if (mpui->current_screen && mpui->current_screen->focus_box)
    mpui_focus_action_exec ((mpui_focus_box_t *) mpui->current_screen->focus_box[0], MPUI_WHEN_VALIDATE);
}

void
mpui_cmd_focus_box_previous (mpui_t *mpui)
{
  if (mpui->current_screen)
    mpui_focus_box_previous (mpui->current_screen);
}

void
mpui_cmd_focus_box_next (mpui_t *mpui)
{
  if (mpui->current_screen)
    mpui_focus_box_next (mpui->current_screen);
}

void
mpui_cmd_focus_previous (mpui_t *mpui)
{
  if (mpui->current_screen && mpui->current_screen->focus_box)
    mpui_focus_previous ((mpui_focus_box_t *) mpui->current_screen->focus_box[0]);
}

void
mpui_cmd_focus_previous_line (mpui_t *mpui)
{
  if (mpui->current_screen && mpui->current_screen->focus_box)
    mpui_focus_previous_line ((mpui_focus_box_t *) mpui->current_screen->focus_box[0]);
}

void
mpui_cmd_focus_next (mpui_t *mpui)
{
  if (mpui->current_screen && mpui->current_screen->focus_box)
    mpui_focus_next ((mpui_focus_box_t *) mpui->current_screen->focus_box[0]);
}

void
mpui_cmd_focus_next_line (mpui_t *mpui)
{
  if (mpui->current_screen && mpui->current_screen->focus_box)
    mpui_focus_next_line ((mpui_focus_box_t *) mpui->current_screen->focus_box[0]);
}


static void
mpui_cmd_info_func (mpui_t *mpui __attribute__((unused)),
                    mpui_element_t *element, void *data)
{
  mpui_info_filename ((mpui_inf_t *) element, (char *) data);
}

void
mpui_cmd_info (mpui_t *mpui, char *filename)
{
  if (!mpui->current_screen)
    return;

  mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                             NULL, MPUI_INF, 0, mpui_cmd_info_func,
                             (void *) filename);
}


static void
mpui_cmd_slideshow_path_func (mpui_t *mpui __attribute__((unused)),
                              mpui_element_t *element, void *data)
{
  mpui_slideshow_path ((mpui_slideshow_t *) element, (char *) data);
}

void
mpui_cmd_slideshow_path (mpui_t *mpui, char *slideshow_id, char *path)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               slideshow_id, MPUI_SLIDESHOW, 0,
                               mpui_cmd_slideshow_path_func, (void *) path);
}

static void
mpui_cmd_slideshow_pause_func (mpui_t *mpui __attribute__((unused)),
                               mpui_element_t *element,
                               void *data __attribute__((unused)))
{
  mpui_slideshow_pause ((mpui_slideshow_t *) element);
}

void
mpui_cmd_slideshow_pause (mpui_t *mpui, char *slideshow_id)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               slideshow_id, MPUI_SLIDESHOW, 0,
                               mpui_cmd_slideshow_pause_func, NULL);
}

static void
mpui_cmd_slideshow_prev_func (mpui_t *mpui __attribute__((unused)),
                              mpui_element_t *element,
                              void *data __attribute__((unused)))
{
  mpui_slideshow_prev ((mpui_slideshow_t *) element);
}

void
mpui_cmd_slideshow_prev (mpui_t *mpui, char *slideshow_id)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               slideshow_id, MPUI_SLIDESHOW, 0,
                               mpui_cmd_slideshow_prev_func, NULL);
}

static void
mpui_cmd_slideshow_next_func (mpui_t *mpui __attribute__((unused)),
                              mpui_element_t *element,
                              void *data __attribute__((unused)))
{
  mpui_slideshow_next ((mpui_slideshow_t *) element);
}

void
mpui_cmd_slideshow_next (mpui_t *mpui, char *slideshow_id)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               slideshow_id, MPUI_SLIDESHOW, 0,
                               mpui_cmd_slideshow_next_func, NULL);
}

static void
mpui_cmd_slideshow_mode_func (mpui_t *mpui __attribute__((unused)),
                              mpui_element_t *element, void *data)
{
  mpui_slideshow_mode ((mpui_slideshow_t *) element, (char *) data);
}

void
mpui_cmd_slideshow_mode (mpui_t *mpui, char *slideshow_id, char *mode)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               slideshow_id, MPUI_SLIDESHOW, 0,
                               mpui_cmd_slideshow_mode_func, mode);
}

static void
mpui_cmd_slideshow_rotate_func (mpui_t *mpui __attribute__((unused)),
                              mpui_element_t *element, void *data)
{
  mpui_slideshow_rotate ((mpui_slideshow_t *) element, *(int *)data);
}

void
mpui_cmd_slideshow_rotate (mpui_t *mpui, char *slideshow_id, int rotate)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               slideshow_id, MPUI_SLIDESHOW, 0,
                               mpui_cmd_slideshow_rotate_func, &rotate);
}


static void
mpui_cmd_playlist_add_func (mpui_t *mpui __attribute__((unused)),
                            mpui_element_t *element, void *data)
{
  mpui_playlist_t *playlist = (mpui_playlist_t*) ((mpui_mnu_t*) element)->menu;
  if (((mpui_mnu_t*) element)->menu->is_playlist)
    mpui_playlist_add (playlist, (char *) data);
}

void
mpui_cmd_playlist_add (mpui_t *mpui, char *playlist_id, char *filename)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               playlist_id, MPUI_MNU, 0,
                               mpui_cmd_playlist_add_func, filename);
}

static void
mpui_cmd_playlist_move_up_func (mpui_t *mpui __attribute__((unused)),
                                mpui_element_t *element,
                                void *data __attribute__((unused)))
{
  mpui_playlist_t *playlist = (mpui_playlist_t*) ((mpui_mnu_t*) element)->menu;
  mpui_element_t **item = ((mpui_focus_box_t *) element)->focus;
  if (((mpui_mnu_t*) element)->menu->is_playlist)
    mpui_playlist_move_up (playlist, item);
}

void
mpui_cmd_playlist_move_up (mpui_t *mpui, char *playlist_id)
{
  if (mpui->current_screen)
    {
      mpui_element_t **fb = mpui->current_screen->focus_box;

      if (playlist_id)
        mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                                   playlist_id, MPUI_MNU, 0,
                                   mpui_cmd_playlist_move_up_func, NULL);
      else if (fb && *fb && (*fb)->type == MPUI_MNU
               && ((mpui_mnu_t *) *fb)->menu->is_playlist)
        mpui_cmd_playlist_move_up_func (NULL, *fb, NULL);
    }
}

static void
mpui_cmd_playlist_move_down_func (mpui_t *mpui __attribute__((unused)),
                                  mpui_element_t *element,
                                  void *data __attribute__((unused)))
{
  mpui_playlist_t *playlist = (mpui_playlist_t*) ((mpui_mnu_t*) element)->menu;
  mpui_element_t **item = ((mpui_focus_box_t *) element)->focus;
  if (((mpui_mnu_t*) element)->menu->is_playlist)
    mpui_playlist_move_down (playlist, item);
}

void
mpui_cmd_playlist_move_down (mpui_t *mpui, char *playlist_id)
{
  if (mpui->current_screen)
    {
      mpui_element_t **fb = mpui->current_screen->focus_box;

      if (playlist_id)
        mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                                   playlist_id, MPUI_MNU, 0,
                                   mpui_cmd_playlist_move_down_func, NULL);
      else if (fb && *fb && (*fb)->type == MPUI_MNU
               && ((mpui_mnu_t *) *fb)->menu->is_playlist)
        mpui_cmd_playlist_move_down_func (NULL, *fb, NULL);
    }
}

static void
mpui_cmd_playlist_remove_func (mpui_t *mpui __attribute__((unused)),
                               mpui_element_t *element, void *data)
{
  mpui_playlist_t *playlist = (mpui_playlist_t*) ((mpui_mnu_t*) element)->menu;
  if (((mpui_mnu_t*) element)->menu->is_playlist)
    mpui_playlist_remove (playlist, (char *) data);
}

void
mpui_cmd_playlist_remove (mpui_t *mpui, char *playlist_id, char *filename)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               playlist_id, MPUI_MNU, 0,
                               mpui_cmd_playlist_remove_func, filename);
}

static void
mpui_cmd_playlist_empty_func (mpui_t *mpui __attribute__((unused)),
                              mpui_element_t *element,
                              void *data __attribute__((unused)))
{
  mpui_playlist_t *playlist = (mpui_playlist_t*) ((mpui_mnu_t*) element)->menu;
  if (((mpui_mnu_t*) element)->menu->is_playlist)
    mpui_playlist_empty (playlist);
}

void
mpui_cmd_playlist_empty (mpui_t *mpui, char *playlist_id)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               playlist_id, MPUI_MNU, 0,
                               mpui_cmd_playlist_empty_func, NULL);
}

static void
mpui_cmd_playlist_load_func (mpui_t *mpui __attribute__((unused)),
                             mpui_element_t *element, void *data)
{
  mpui_playlist_t *playlist = (mpui_playlist_t*) ((mpui_mnu_t*) element)->menu;
  if (((mpui_mnu_t*) element)->menu->is_playlist)
    mpui_playlist_load (playlist, (char *) data);
}

void
mpui_cmd_playlist_load (mpui_t *mpui, char *playlist_id, char *filename)
{
  if (mpui->current_screen)
    mpui_cmd_for_each_element (mpui, mpui->current_screen->elements,
                               playlist_id, MPUI_MNU, 0,
                               mpui_cmd_playlist_load_func, filename);
}
