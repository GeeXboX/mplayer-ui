/*
 *  mpui_render.c: libmpui screen renderer.
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
#include "mpui_render.h"

#include "../libmpcodecs/img_format.h"
#include "../libvo/fastmemcpy.h"


static inline void
mpui_render_alpha (unsigned char *dst, unsigned char src, unsigned char alpha)
{
  *dst = alpha ? src + (*dst * (alpha+1) >> 8) : src;
}

static void
mpui_render_planar_image (mpui_image_t *image, mp_image_t *mpi)
{
  unsigned char *end, *y, *u, *v, *yi, *ui, *vi, *aiy, *aiuv;
  int i, incr;

  yi   = image->planes[0];
  ui   = image->planes[1];
  vi   = image->planes[2];
  aiy  = image->planes[3];
  aiuv = image->planes[4];

  y = mpi->planes[0] + image->y*mpi->stride[0] + image->x;
  u = mpi->planes[1] + (image->y>>mpi->chroma_y_shift)*mpi->stride[1]
                     + (image->x>>mpi->chroma_x_shift);
  v = mpi->planes[2] + (image->y>>mpi->chroma_y_shift)*mpi->stride[2]
                     + (image->x>>mpi->chroma_x_shift);

  if (image->alpha)
    {
      incr = mpi->stride[0] - image->w;
      for (end=yi+image->w*image->h; yi<end; y+=incr)
        for (i=0; i<image->w; i++, y++, yi++, aiy++)
            mpui_render_alpha (y, *yi, *aiy);
      incr = mpi->stride[1] - image->chroma_width;
      for (end=ui+image->chroma_width*image->chroma_height; ui<end; u+=incr, v+=incr)
        for (i=0; i<image->chroma_width; i++, u++, v++, ui++, vi++, aiuv++)
          {
            mpui_render_alpha (u, *ui, *aiuv);
            mpui_render_alpha (v, *vi, *aiuv);
          }
    }
  else
    {
      for (end=yi+image->w*image->h; yi<end;
           y+=mpi->stride[0], yi+=image->w)
        memcpy (y, yi, image->w);
      for (end=ui+image->chroma_width*image->chroma_height; ui<end;
           u+=mpi->stride[1], v+=mpi->stride[2],
           ui+=image->chroma_width, vi+=image->chroma_width)
        {
          memcpy (u, ui, image->chroma_width);
          memcpy (v, vi, image->chroma_width);
        }
    }
}

static void
mpui_render_packed_image (mpui_image_t *image, mp_image_t *mpi)
{
  unsigned char *end, *y, *yi, *aiy;
  int i, incr;

  yi  = image->planes[0];
  y = mpi->planes[0] + image->y*mpi->stride[0] + image->x*image->bpp;

  if (image->alpha)
    {
      aiy = image->planes[3];
      incr = mpi->stride[0] - image->stride[0];
      for (end=yi+image->bpp*image->w*image->h; yi<end; y+=incr)
        for (i=0; i<(image->w>>1); i++, aiy+=2)
          {
            mpui_render_alpha (y++, *yi++, *aiy);
            mpui_render_alpha (y++, *yi++, *aiy);
            mpui_render_alpha (y++, *yi++, *(aiy+1));
            mpui_render_alpha (y++, *yi++, *aiy);
          }
    }
  else
    {
      for (end=yi+image->stride[0]*image->h; yi<end;
           y+=mpi->stride[0], yi+=image->stride[0])
        memcpy (y, yi, image->stride[0]);
    }
}

static inline void
mpui_render_image (mpui_image_t *image, mp_image_t *mpi)
{
  if (image->num_planes <= 0)
    return;

  switch (mpi->imgfmt)
    {
    case IMGFMT_YVU9:
    case IMGFMT_YV12:
    case IMGFMT_I420:
    case IMGFMT_IYUV:
      mpui_render_planar_image (image, mpi);
      break;
    case IMGFMT_YUY2:
    case IMGFMT_UYVY:
      mpui_render_packed_image (image, mpi);
      break;
    }
}

static inline void
mpui_render_element (mpui_element_t *element, mp_image_t *mpi)
{
  if (!element)
    return;

  switch (element->type)
    {
    case MPUI_IMG:
      mpui_render_image (element->img->image, mpi);
      break;
    case MPUI_OBJ:
      mpui_render_object (element->obj->object, mpi);
      break;

    default:
      break;
    }
}

void
mpui_render_object (mpui_object_t *object, mp_image_t *mpi)
{
  mpui_element_t **elements;

  for (elements=object->elements; *elements; elements++)
    mpui_render_element (*elements, mpi);
}

int
mpui_render_screen (mpui_screen_t *screen, mp_image_t *mpi)
{
  mpui_element_t **elements;

  for (elements=screen->elements; *elements; elements++)
    mpui_render_element (*elements, mpi);

  return 0;
}
