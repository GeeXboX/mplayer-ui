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
#include "../libmpcodecs/img_format.h"
#include "../libmpcodecs/vf.h"
#include "../libvo/fastmemcpy.h"

#include "mpui_parser.h"
#include "mpui_render.h"


struct vf_priv_s {
  mpui_t *mpui;
};


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
put_image (struct vf_instance_s* vf, mp_image_t *mpi)
{
  mp_image_t *dmpi = mpi;

  if (vf->priv->mpui && vf->priv->mpui->screens && vf->priv->mpui->screens->menu)
    {
      dmpi = vf_get_image (vf->next, mpi->imgfmt, MP_IMGTYPE_TEMP, 0,
                           mpi->w, mpi->h);
      copy_mpi(dmpi, mpi);
      mpui_render_screen (vf->priv->mpui->screens->menu, dmpi);
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
  vf->priv->mpui = mpui_parse_config_file ("mpui-theme.xml",
                                           width, height, outfmt);
  return vf_next_config (vf, width, height, d_width, d_height, flags, outfmt);
}

static int
open (vf_instance_t *vf, char* args)
{
  vf->query_format = query_format;
  vf->config = config;
  vf->put_image = put_image;
  //  vf->get_image = get_image;
  vf->uninit = uninit;

  vf->priv = (struct vf_priv_s *) malloc (sizeof (struct vf_priv_s));
  vf->priv->mpui = NULL;

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
