/*
 *  vf_mpui.c: libmpui video filter.
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

#include "../config.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "../libmpcodecs/mp_image.h"
#include "../libmpcodecs/vf.h"

#include "mpui_parser.h"


struct vf_priv_s {
  mpui_t *mpui;
};


static int
put_image (struct vf_instance_s* vf, mp_image_t *mpi)
{
  return vf_next_put_image (vf, mpi);
}

static void
uninit (vf_instance_t *vf)
{
  mpui_free (vf->priv->mpui);
  free (vf->priv);
  vf->priv = NULL;
}

static int
config (struct vf_instance_s* vf, int width, int height,
        int d_width, int d_height, unsigned int flags, unsigned int outfmt)
{
  return vf_next_config (vf, width, height, d_width, d_height, flags, outfmt);
}

static int
open (vf_instance_t *vf, char* args)
{
  struct vf_priv_s* st_priv;

  vf->config = config;
  vf->put_image = put_image;
  //  vf->get_image = get_image;
  vf->uninit = uninit;

  st_priv = (struct vf_priv_s *) malloc (sizeof (*st_priv));
  st_priv->mpui = mpui_parse_config_file ("mpui-theme.xml");
  vf->priv = st_priv;

  return 1;
}


vf_info_t vf_info_mpui = {
  "Internal filter for libmpui",
  "mpui",
  "Aurelien Jacobs\nBenjamin Zores",
  "",
  open,
  NULL
};
