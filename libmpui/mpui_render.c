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
#include "../libvo/osd.h"
#include "../libvo/font_load.h"
#include "../mp_msg.h"

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

typedef void (*draw_alpha_f)
(int w, int h, unsigned char* src, unsigned char *srca,
 int srcstride, unsigned char* dstbase,int dststride);

inline static draw_alpha_f
get_draw_alpha (int fmt)
{
  switch (fmt)
    {
    case IMGFMT_BGR15:
    case IMGFMT_RGB15:
      return vo_draw_alpha_rgb15;
    case IMGFMT_BGR16:
    case IMGFMT_RGB16:
      return vo_draw_alpha_rgb16;
    case IMGFMT_BGR24:
    case IMGFMT_RGB24:
      return vo_draw_alpha_rgb24;
    case IMGFMT_BGR32:
    case IMGFMT_RGB32:
      return vo_draw_alpha_rgb32;
    case IMGFMT_YV12:
    case IMGFMT_I420:
    case IMGFMT_IYUV:
    case IMGFMT_YVU9:
    case IMGFMT_IF09:
    case IMGFMT_Y800:
    case IMGFMT_Y8:
      return vo_draw_alpha_yv12;
    case IMGFMT_YUY2:
      return vo_draw_alpha_yuy2;
    }

  return NULL;
}
   
static inline void
mpui_render_string (mpui_str_t *str, mp_image_t* mpi)
{
  draw_alpha_f draw_alpha = get_draw_alpha (mpi->imgfmt);
  char *p, *txt = str->string->text;
  int x = str->x, y = str->y;
  int font;  

  if (!draw_alpha)
    {
      mp_msg (MSGT_OSD, MSGL_ERR, "Unsupported outformat !!!!\n");
      return;
    }
  
  p = txt;
  while (*p)
    render_one_glyph (vo_font, *p++);

  while (*txt)
    {
      unsigned char c = *txt++;
      if ((font = vo_font->font[c]) >= 0
          && (x + vo_font->width[c] <= mpi->w)
          && (y + vo_font->pic_a[font]->h <= mpi->h))
        draw_alpha (vo_font->width[c], vo_font->pic_a[font]->h,
                    vo_font->pic_b[font]->bmp+vo_font->start[c],
                    vo_font->pic_a[font]->bmp+vo_font->start[c],
                    vo_font->pic_a[font]->w,
                    mpi->planes[0] + y * mpi->stride[0] + x * (mpi->bpp>>3),
                    mpi->stride[0]);
      x += vo_font->width[c]+vo_font->charspace;
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
      if (element->img->when_focused == MPUI_DISPLAY_ALWAYS
          || (element->focus 
              && element->img->when_focused == MPUI_DISPLAY_FOCUSED)
          || (!element->focus 
              && element->img->when_focused == MPUI_DISPLAY_NORMAL))
        mpui_render_image (element->img->image, mpi);
      break;
    case MPUI_OBJ:
      if (element->obj->when_focused == MPUI_DISPLAY_ALWAYS
          || (element->focus 
              && element->obj->when_focused == MPUI_DISPLAY_FOCUSED)
          || (!element->focus 
              && element->obj->when_focused == MPUI_DISPLAY_NORMAL))
        mpui_render_object (element->obj->object, mpi);
      break;
    case MPUI_STR:
      if (element->str->when_focused == MPUI_DISPLAY_ALWAYS
          || (element->focus 
              && element->str->when_focused == MPUI_DISPLAY_FOCUSED)
          || (!element->focus 
              && element->str->when_focused == MPUI_DISPLAY_NORMAL))
        mpui_render_string (element->str, mpi);
      break;
    case MPUI_MNU:
      mpui_render_menu (element->mnu->menu, mpi);
      break;
    case MPUI_MENUITEM:
      mpui_render_menuitem (element->menuitem, mpi, element->focus);
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

void
mpui_render_menu (mpui_menu_t *menu, mp_image_t *mpi)
{
  mpui_element_t **elements;
  int first = 0; /* Only set focus to first menuitem */

  for (elements=menu->elements; *elements; elements++)
    {
      if ((*elements)->type == MPUI_MENUITEM && !first)
        {
          (*elements)->focus = 1;
          first++;
        }
      mpui_render_element (*elements, mpi);
    }
}

void
mpui_render_menuitem (mpui_menuitem_t *menuitem, mp_image_t *mpi, int focus)
{
  mpui_element_t **elements;

  for (elements=menuitem->elements; *elements; elements++)
    {
      if (focus)
        (*elements)->focus = 1;
      mpui_render_element (*elements, mpi);
    }
}

int
mpui_render_screen (mpui_screen_t *screen, mp_image_t *mpi)
{
  mpui_element_t **elements;

  for (elements=screen->elements; *elements; elements++)
    mpui_render_element (*elements, mpi);

  return 0;
}
