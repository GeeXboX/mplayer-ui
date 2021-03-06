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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
#include "mp_msg.h"
#include "mplayer.h"
#include "m_option.h"
#include "m_struct.h"

#include "mpui_focus.h"
#include "mpui_browser.h"
#include "mpui_playlist.h"
#include "mpui_parser.h"
#include "mpui_render.h"
#include "mpui_cmd.h"


static struct vf_priv_s {
  mpui_t *mpui;
  char *theme;
  char *lang;
  int show;
} vf_priv_dflt = {
  NULL,
  "default",
  "",
  1
};


static int
query_format (struct vf_instance_s* vf __attribute__((unused)),
              unsigned int fmt)
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
cmd_filter (mp_cmd_t *cmd, int paused __attribute__((unused)),
            struct vf_priv_s *priv)
{
  int i;

  switch (cmd->id)
    {
    case MP_CMD_MPUI_POPUP:
      mpui_cmd_popup (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_POPUP_CLOSE:
      mpui_cmd_popup_close (priv->mpui);
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
    case MP_CMD_MPUI_HIDE_SWITCH:
      mpui_cmd_hide_switch (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_FOCUS_ACTION_EXEC:
      mpui_cmd_focus_action_exec (priv->mpui);
      return 1;
    case MP_CMD_MPUI_FOCUS_BOX_PREVIOUS:
      mpui_cmd_focus_box_previous (priv->mpui);
      return 1;
    case MP_CMD_MPUI_FOCUS_BOX_NEXT:
      mpui_cmd_focus_box_next (priv->mpui);
      return 1;
    case MP_CMD_MPUI_FOCUS_PREVIOUS:
      mpui_cmd_focus_previous (priv->mpui);
      return 1;
    case MP_CMD_MPUI_FOCUS_PREVIOUS_LINE:
      mpui_cmd_focus_previous_line (priv->mpui);
      return 1;
    case MP_CMD_MPUI_FOCUS_NEXT:
      mpui_cmd_focus_next (priv->mpui);
      return 1;
    case MP_CMD_MPUI_FOCUS_NEXT_LINE:
      mpui_cmd_focus_next_line (priv->mpui);
      return 1;
    case MP_CMD_MPUI_INFO:
      mpui_cmd_info (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_SLIDESHOW_PATH:
      mpui_cmd_slideshow_path (priv->mpui, cmd->args[0].v.s,cmd->args[1].v.s);
      return 1;
    case MP_CMD_MPUI_SLIDESHOW_PAUSE:
      mpui_cmd_slideshow_pause (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_SLIDESHOW_PREV:
      mpui_cmd_slideshow_prev (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_SLIDESHOW_NEXT:
      mpui_cmd_slideshow_next (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_SLIDESHOW_MODE:
      mpui_cmd_slideshow_mode (priv->mpui, cmd->args[0].v.s, cmd->args[1].v.s);
      return 1;
    case MP_CMD_MPUI_SLIDESHOW_ROTATE:
      i = cmd->nargs > 1 ? cmd->args[1].v.i : 1;
      mpui_cmd_slideshow_rotate (priv->mpui, cmd->args[0].v.s, i);
      return 1;
    case MP_CMD_MPUI_PLAYLIST_ADD:
      mpui_cmd_playlist_add (priv->mpui, cmd->args[0].v.s, cmd->args[1].v.s);
      return 1;
    case MP_CMD_MPUI_PLAYLIST_MOVE_UP:
      mpui_cmd_playlist_move_up (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_PLAYLIST_MOVE_DOWN:
      mpui_cmd_playlist_move_down (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_PLAYLIST_REMOVE:
      mpui_cmd_playlist_remove (priv->mpui, cmd->args[0].v.s,cmd->args[1].v.s);
      return 1;
    case MP_CMD_MPUI_PLAYLIST_EMPTY:
      mpui_cmd_playlist_empty (priv->mpui, cmd->args[0].v.s);
      return 1;
    case MP_CMD_MPUI_PLAYLIST_LOAD:
      mpui_cmd_playlist_load (priv->mpui, cmd->args[0].v.s, cmd->args[1].v.s);
      return 1;
    }
  return 0;
}

static int
put_image (struct vf_instance_s* vf, mp_image_t *mpi)
{
  mp_image_t *dmpi = mpi;

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
  mp_msg (MSGT_VFILTER, MSGL_INFO,
          "mpui: Using language '%s'\n", vf->priv->lang);

  vf->priv->mpui = mpui_parse_config_file (vf->priv->theme, vf->priv->lang,
                                           width, height, outfmt);

  return vf_next_config (vf, width, height, d_width, d_height, flags, outfmt);
}

static int
vf_open (vf_instance_t *vf, char* args __attribute__((unused)))
{
  vf->query_format = query_format;
  vf->config = config;
  vf->put_image = put_image;
  //  vf->get_image = get_image;
  vf->uninit = uninit;

  if (!vf->priv)
    {
      vf->priv = (struct vf_priv_s *) malloc (sizeof (struct vf_priv_s));
      vf->priv->mpui = NULL;
      vf->priv->theme = strdup ("default");
      vf->priv->lang = strdup ("");
      vf->priv->show = 1;
    }

  mp_input_add_cmd_filter ((mp_input_cmd_filter) cmd_filter, vf->priv);

  return 1;
}

#define ST_OFF(f)  M_ST_OFF (struct vf_priv_s, f)
static m_option_t vf_opts_fields[] = {
  {"theme", ST_OFF (theme), CONF_TYPE_STRING, 0, 0, 0, NULL},
  {"lang", ST_OFF (lang), CONF_TYPE_STRING, 0, 0, 0, NULL},
  { NULL, NULL, 0, 0, 0, 0,  NULL }
};

static m_struct_t vf_opts = {
  "mpui",
  sizeof (struct vf_priv_s),
  &vf_priv_dflt,
  vf_opts_fields
};

vf_info_t vf_info_mpui = {
  "Internal filter for libmpui",
  "mpui",
  "Aurelien Jacobs\nBenjamin Zores",
  "",
  vf_open,
  &vf_opts
};
