/*
 *  vf_mpui.c: libmpui video filter.
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "libmpcodecs/mp_image.h"
#include "libmpcodecs/img_format.h"
#include "libmpcodecs/vf.h"
#include "libvo/fastmemcpy.h"
#include "input/input.h"
#include "osdep/keycodes.h"
#include "mp_msg.h"
#include "mplayer.h"

#include "mpui_focus.h"
#include "mpui_browser.h"
#include "mpui_parser.h"
#include "mpui_render.h"
#include "mpui_cmd.h"


struct vf_priv_s {
  mpui_t *mpui;
  char *theme;
  int show;
};

static struct vf_priv_s *st_priv = NULL;


static int
query_format (struct vf_instance_s* vf, unsigned int fmt)
{
  switch (fmt)
    {
    case IMGFMT_YVU9:
    case IMGFMT_YV12:
    case IMGFMT_I420:
    case IMGFMT_IYUV:
    case IMGFMT_YUY2:
    case IMGFMT_UYVY:
      return VFCAP_CSP_SUPPORTED;
    }
  return 0;
}

static inline void 
copy_mpi(mp_image_t *dmpi, mp_image_t *mpi) 
{
  if (mpi->flags & MP_IMGFLAG_PLANAR)
    {
      memcpy_pic (dmpi->planes[0], mpi->planes[0], mpi->w, mpi->h, 
                  dmpi->stride[0], mpi->stride[0]);
      memcpy_pic (dmpi->planes[1], mpi->planes[1], mpi->chroma_width, 
                  mpi->chroma_height, dmpi->stride[1], mpi->stride[1]);
      memcpy_pic (dmpi->planes[2], mpi->planes[2], mpi->chroma_width, 
                  mpi->chroma_height, dmpi->stride[2], mpi->stride[2]);
    }
  else
    {
      memcpy_pic (dmpi->planes[0], mpi->planes[0], mpi->w*(dmpi->bpp/8), 
                  mpi->h, dmpi->stride[0], mpi->stride[0]); 
    }
}

static int
cmd_filter (mp_cmd_t *cmd, int paused, struct vf_priv_s *priv)
{
  switch (cmd->id)
    {
    case MP_CMD_MPUI_POPUP:
      mpui_cmd_popup (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_SCREEN:
      mpui_cmd_screen (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_CD:
      mpui_browser_cd (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_HIDE:
      mpui_cmd_hide (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_SHOW:
      mpui_cmd_show (priv->mpui, cmd->args[0].v.s);
      return 1;
    }
  return 0;
}

static void
read_keycode (int code)
{
  mpui_focus_box_t *fb;

  if (!st_priv->mpui->current_screen)
    return;

  fb = (mpui_focus_box_t *) st_priv->mpui->current_screen->focus_box[0];

  if (mpui_list_empty (st_priv->mpui->current_screen->popup_stack))
    switch (code)
      {
      case KEY_UP:
        if (fb->orientation == MPUI_ORIENTATION_V
            || (fb->orientation & MPUI_ORIENTATION_V
                && fb->scrolling == MPUI_ORIENTATION_H))
          mpui_focus_previous (fb);
        else if (fb->orientation & MPUI_ORIENTATION_V
                 && fb->scrolling == MPUI_ORIENTATION_V)
          mpui_focus_previous_line (fb);
        break;
      case KEY_DOWN:
        if (fb->orientation == MPUI_ORIENTATION_V
            || (fb->orientation & MPUI_ORIENTATION_V
                && fb->scrolling == MPUI_ORIENTATION_H))
          mpui_focus_next (fb);
        else if (fb->orientation & MPUI_ORIENTATION_V
                 && fb->scrolling == MPUI_ORIENTATION_V)
          mpui_focus_next_line (fb);
        break;
      case KEY_LEFT:
        if (fb->orientation == MPUI_ORIENTATION_H
            || (fb->orientation & MPUI_ORIENTATION_H
                && fb->scrolling == MPUI_ORIENTATION_V))
          mpui_focus_previous (fb);
        else if (fb->orientation & MPUI_ORIENTATION_H
                 && fb->scrolling == MPUI_ORIENTATION_H)
          mpui_focus_previous_line (fb);
        break;
      case KEY_RIGHT:
        if (fb->orientation == MPUI_ORIENTATION_H
            || (fb->orientation & MPUI_ORIENTATION_H
                && fb->scrolling == MPUI_ORIENTATION_V))
          mpui_focus_next (fb);
        else if (fb->orientation & MPUI_ORIENTATION_H
                 && fb->scrolling == MPUI_ORIENTATION_H)
          mpui_focus_next_line (fb);
        break;
      case KEY_TAB:
      case KEY_SPACE:
        mpui_focus_box_next (st_priv->mpui->current_screen);
        break;
      case KEY_ENTER:
        mpui_focus_action_exec (fb, MPUI_WHEN_VALIDATE);
        break;
      case KEY_ESC:
        if (st_priv->mpui->previous_screen)
          {
            st_priv->mpui->current_screen = st_priv->mpui->previous_screen;
            st_priv->mpui->previous_screen = NULL;
            mpui_focus_box_first (st_priv->mpui->current_screen);
          }
        else
          mp_input_queue_cmd (mp_input_parse_cmd ("quit"));
      }
  else
    switch (code)
      {
      case KEY_ESC:
        mpui_cmd_popup_close (st_priv->mpui);
      }
}

static int
put_image (struct vf_instance_s* vf, mp_image_t *mpi)
{
  mp_image_t *dmpi = mpi;

  /* (Un)Grab the keys */
  if (!mp_input_key_cb && vf->priv->show)
    mp_input_key_cb = read_keycode;
  if (mp_input_key_cb && !vf->priv->show)
    mp_input_key_cb = NULL;

  if (vf->priv->mpui && vf->priv->mpui->screens && vf->priv->mpui->current_screen)
    {
      dmpi = vf_get_image (vf->next, mpi->imgfmt, MP_IMGTYPE_TEMP, 0,
                           mpi->w, mpi->h);
      copy_mpi(dmpi, mpi);
      mpui_render_screen (vf->priv->mpui, dmpi);
    }

  return vf_next_put_image (vf, dmpi);
}

static void
uninit (vf_instance_t *vf)
{
  if (vf->priv->mpui)
    mpui_free (vf->priv->mpui);
  free (vf->priv);
  vf->priv = NULL;
}

static int
config (struct vf_instance_s* vf, int width, int height,
        int d_width, int d_height, unsigned int flags, unsigned int outfmt)
{
  mp_msg (MSGT_VFILTER, MSGL_INFO,
          "mpui: Using theme '%s'\n", vf->priv->theme);

  vf->priv->mpui = mpui_parse_config_file (vf->priv->theme,
                                           width, height, outfmt);

  return vf_next_config (vf, width, height, d_width, d_height, flags, outfmt);
}

static int
vf_open (vf_instance_t *vf, char* args)
{
  vf->query_format = query_format;
  vf->config = config;
  vf->put_image = put_image;
  //  vf->get_image = get_image;
  vf->uninit = uninit;

  vf->priv = (struct vf_priv_s *) malloc (sizeof (struct vf_priv_s));
  vf->priv->mpui = NULL;

  vf->priv->theme = (char *) malloc (128 * sizeof (char));
  if (args)
    sscanf (args, "%s", vf->priv->theme);
  else
    strcpy (vf->priv->theme, "default");

  vf->priv->show = 1;
  st_priv = vf->priv;

  mp_input_add_cmd_filter ((mp_input_cmd_filter) cmd_filter, vf->priv);

  return 1;
}


vf_info_t vf_info_mpui = {
  "Internal filter for libmpui",
  "mpui",
  "Aurelien Jacobs\nBenjamin Zores",
  "",
  vf_open,
  NULL
};
