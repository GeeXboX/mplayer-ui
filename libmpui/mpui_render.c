/*
 *  mpui_render.c: libmpui screen renderer.
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
#include "mpui_browser.h"
#include "mpui_playlist.h"
#include "mpui_info.h"
#include "mpui_slideshow.h"
#include "mpui_focus.h"
#include "mpui_parser.h"
#include "mpui_render.h"

#include "libmpcodecs/img_format.h"
#include "libvo/fastmemcpy.h"
#include "libvo/font_load.h"


typedef struct mpui_render_context mpui_render_context_t;

struct mpui_render_context {
  mpui_t *mpui;
  mpui_screen_t *screen;
  int focus, really_focus;
  mpui_size_t x, y;
};


static void
mpui_render_context_update (mpui_element_t *element,
                            mpui_render_context_t *context, int remove)
{
  if (element->flags & MPUI_FLAG_ABSOLUTE)
    {
      context->x = element->x.val;
      context->y = element->y.val;
    }
  else if (remove)
    {
      context->x -= element->x.val;
      context->y -= element->y.val;
    }
  else
    {
      context->x += element->x.val;
      context->y += element->y.val;
    }
}

static void
mpui_render_context_update_offset (mpui_focus_box_t *focus_box,
                                   mpui_render_context_t *context, int remove)
{
  if (remove)
    {
      context->x += focus_box->xoffset;
      context->y += focus_box->yoffset;
    }
  else
    {
      context->x -= focus_box->xoffset;
      context->y -= focus_box->yoffset;
    }
}


static inline void
mpui_render_alpha (unsigned char *dst, unsigned char src, unsigned char alpha)
{
  *dst = alpha ? src + (*dst * (alpha+1) >> 8) : src;
}

static void
mpui_render_planar_image (mpui_img_t *img, mp_image_t *mpi,
                          mpui_render_context_t *context)
{
  unsigned char *end, *y, *u, *v, *yi, *ui, *vi, *aiy, *aiuv;
  mpui_image_t *image = img->image;
  int i, incr, incri, cx, cy, cw, ch, ccw, cch;

  cx = context->x + img->cx1;
  cy = context->y + img->cy1;
  cw = image->w - (img->cx1 + img->cx2);
  ch = image->h - (img->cy1 + img->cy2);
  ccw = cw >> mpi->chroma_x_shift;
  cch = ch >> mpi->chroma_y_shift;

  if (cw <= 0 || ch <= 0)
    return;

  yi   = image->planes[0] + img->cy1*image->stride[0] + img->cx1;
  ui   = image->planes[1] + (img->cy1 >> mpi->chroma_y_shift)*image->stride[1]
                          + (img->cx1 >> mpi->chroma_x_shift);
  vi   = image->planes[2] + (img->cy1 >> mpi->chroma_y_shift)*image->stride[2]
                          + (img->cx1 >> mpi->chroma_x_shift);
  aiy  = image->planes[3] + img->cy1*image->stride[3] + img->cx1;
  aiuv = image->planes[4] + (img->cy1 >> mpi->chroma_y_shift)*image->stride[4]
                          + (img->cx1 >> mpi->chroma_x_shift);

  y = mpi->planes[0] + cy*mpi->stride[0] + cx;
  u = mpi->planes[1] + (cy>>mpi->chroma_y_shift)*mpi->stride[1]
                     + (cx>>mpi->chroma_x_shift);
  v = mpi->planes[2] + (cy>>mpi->chroma_y_shift)*mpi->stride[2]
                     + (cx>>mpi->chroma_x_shift);

  if (image->alpha)
    {
      incr = mpi->stride[0] - cw;
      incri = img->cx1 + img->cx2;
      for (end=yi+image->w*ch; yi<end; y+=incr, yi+=incri, aiy+=incri)
        for (i=0; i<cw; i++, y++, yi++, aiy++)
            mpui_render_alpha (y, *yi, *aiy);
      incr = mpi->stride[1] - ccw;
      incri = (img->cx1 + img->cx2) >> mpi->chroma_x_shift;
      for (end=ui+image->chroma_width*cch; ui<end;
           u+=incr, v+=incr, ui+=incri, vi+=incri, aiuv+=incri)
        for (i=0; i<ccw; i++, u++, v++, ui++, vi++, aiuv++)
          {
            mpui_render_alpha (u, *ui, *aiuv);
            mpui_render_alpha (v, *vi, *aiuv);
          }
    }
  else
    {
      for (end=yi+image->w*ch; yi<end; y+=mpi->stride[0], yi+=image->w)
        memcpy (y, yi, cw);
      for (end=ui+image->chroma_width*cch; ui<end;
           u+=mpi->stride[1], v+=mpi->stride[2],
           ui+=image->chroma_width, vi+=image->chroma_width)
        {
          memcpy (u, ui, ccw);
          memcpy (v, vi, ccw);
        }
    }
}

static void
mpui_render_packed_image (mpui_img_t *img, mp_image_t *mpi,
                          mpui_render_context_t *context)
{
  unsigned char *end, *y, *yi, *aiy;
  mpui_image_t *image = img->image;
  int i, incr, incri, cx, cy, cw, ch;

  cx = (context->x + img->cx1) & ~1;
  cy = (context->y + img->cy1) & ~1;
  cw = image->w - (img->cx1 + img->cx2);
  ch = image->h - (img->cy1 + img->cy2);

  if (cw <= 0 || ch <= 0)
    return;

  yi = image->planes[0] + img->cy1*image->stride[0] + (img->cx1&~1)*image->bpp;
  y = mpi->planes[0] + cy*mpi->stride[0] + cx*image->bpp;

  if (image->alpha)
    {
      aiy = image->planes[3] + img->cy1*image->stride[3] + img->cx1;
      incr = mpi->stride[0] - cw*image->bpp;
      incri = img->cx1 + img->cx2;
      for (end=yi+image->bpp*image->w*ch; yi<end;
           y+=incr, yi+=image->bpp*incri, aiy+=incri)
        {
          for (i=0; i<(cw>>1); i++, aiy+=2)
            {
              mpui_render_alpha (y++, *yi++, *aiy);
              mpui_render_alpha (y++, *yi++, *aiy);
              mpui_render_alpha (y++, *yi++, *(aiy+1));
              mpui_render_alpha (y++, *yi++, *aiy);
            }
          if (cw & 1)
            {
              mpui_render_alpha (y++, *yi++, *aiy);
              mpui_render_alpha (y++, *yi++, *aiy++);
            }
        }
    }
  else
    {
      for (end=yi+image->stride[0]*ch; yi<end;
           y+=mpi->stride[0], yi+=image->stride[0])
        memcpy (y, yi, cw*image->bpp);
    }
}

static inline void
mpui_render_image (mpui_img_t *img, mp_image_t *mpi,
                   mpui_render_context_t *context)
{
  if (img->image->num_planes <= 0)
    return;

  switch (mpi->imgfmt)
    {
    case IMGFMT_YVU9:
    case IMGFMT_YV12:
    case IMGFMT_I420:
    case IMGFMT_IYUV:
      mpui_render_planar_image (img, mpi, context);
      break;
    case IMGFMT_YUY2:
    case IMGFMT_UYVY:
      mpui_render_packed_image (img, mpi, context);
      break;
    }
}

static inline void
mpui_render_planar_glyph (font_desc_t *font, unsigned int c,
                          int pos_x, int pos_y, mpui_color_t *color,
                          mp_image_t *mpi)
{
  unsigned char *src, *srca, *y, *u, *v, *end;
  int i, w, h, incr, f = font->font[c];

  w = font->pic_a[f]->w;
  h = font->pic_a[f]->h;
  src = font->pic_b[f]->bmp + font->start[c];
  srca = font->pic_a[f]->bmp + font->start[c];

  pos_x = (pos_x+1) & ~1;
  pos_y = (pos_y+1) & ~1;

  y = mpi->planes[0] + pos_y*mpi->stride[0] + pos_x;
  incr = mpi->stride[0] - w;

  if (!color)
    {
      for (end=src+w*h; src<end; y+=incr)
        for (i=0; i<w; i++, y++, src++, srca++)
          if (*srca)
            *y = ((*y * *srca)>>8) + *src;
    }
  else
    {
      int x_block = 1 << mpi->chroma_x_shift;
      int y_block = 1 << mpi->chroma_y_shift;

      for (end=src+w*h; src<end; y+=incr)
        for (i=0; i<w; i++, y++, src++, srca++)
          if (*srca)
            *y = ((*y * *srca) + (*src * color->y)) >> 8;

      u = mpi->planes[1] + (pos_y>>mpi->chroma_y_shift)*mpi->stride[1]
                         + (pos_x>>mpi->chroma_x_shift);
      v = mpi->planes[2] + (pos_y>>mpi->chroma_y_shift)*mpi->stride[2]
                         + (pos_x>>mpi->chroma_x_shift);
      src = font->pic_b[f]->bmp + font->start[c];
      incr = mpi->stride[1] - (w>>mpi->chroma_x_shift);
      for (end=src+w*h; src<end; src+=(y_block-1)*w, u+=incr, v+=incr)
        for (i=0; i<w; i+=x_block, u++, v++, src+=x_block)
          {
            int j, k, s = 0;
            for (j=0; j<y_block; j++)
              for (k=0; k<x_block; k++)
                s += src[j*w + k];
            if (s > 300)
              {
                *u = color->u;
                *v = color->v;
              }
          }
    }
}

static inline void
mpui_render_packed_glyph (font_desc_t *font, unsigned int c,
                          int pos_x, int pos_y, mpui_color_t *color,
                          mp_image_t *mpi)
{
  unsigned char *src, *srca, *dst, *end;
  int i, w, h, incr, f = font->font[c];
  int bpp = mpi->bpp >> 3;

  w = font->pic_a[f]->w;
  h = font->pic_a[f]->h;
  src = font->pic_b[f]->bmp + font->start[c];
  srca = font->pic_a[f]->bmp + font->start[c];

  pos_x = (pos_x+1) & ~1;
  dst = mpi->planes[0] + pos_y*mpi->stride[0] + bpp*pos_x;
  incr = mpi->stride[0] - bpp*w;

  if (!color)
    {
      if (mpi->imgfmt == IMGFMT_UYVY)
        dst += 1;
      for (end=src+w*h; src<end; dst+=incr)
        for (i=0; i<w; i++, dst+=bpp, src++, srca++)
          if (*srca)
            *dst = ((*dst * *srca)>>8) + *src;
    }
  else
    {
      int l=0, c=1;

      if (mpi->imgfmt == IMGFMT_UYVY)
        {
          l = 1;
          c = 0;
        }

      for (end=src+w*h; src<end; dst+=incr)
        for (i=0; i<w; i+=bpp, dst+=bpp, src++, srca++)
          {
            int s = *src;
            if (*srca)
              *(dst+l) = ((*(dst+l) * *srca) + (*src * color->y)) >> 8;
            src++;
            srca++;
            s += *src;
            if (s > 150)
              *(dst+c) = color->u;
            dst += bpp;
            if (*srca)
              *(dst+l) = ((*(dst+l) * *srca) + (*src * color->y)) >> 8;
            if (s > 150)
              *(dst+c) = color->v;
          }
    }
}

static inline void
mpui_render_glyph (font_desc_t *font, unsigned int c,
                   int pos_x, int pos_y, mpui_color_t *color,
                   mp_image_t *mpi)
{
  switch (mpi->imgfmt)
    {
    case IMGFMT_YVU9:
    case IMGFMT_YV12:
    case IMGFMT_I420:
    case IMGFMT_IYUV:
      mpui_render_planar_glyph (font, c, pos_x, pos_y, color, mpi);
      break;
    case IMGFMT_YUY2:
    case IMGFMT_UYVY:
      mpui_render_packed_glyph (font, c, pos_x, pos_y, color, mpi);
      break;
    }
}

static inline void
mpui_render_string (mpui_str_t *str, mp_image_t* mpi,
                    mpui_render_context_t *context)
{
  unsigned char *txt = str->string->text;
  font_desc_t *font;
  mpui_color_t *color;
  unsigned int c;
  int f;
  int x_init = context->x, x_end = context->x + ((mpui_element_t *)str)->w.val;

  if (!str->font || !str->font->font_desc)
    return;
  font = str->font->font_desc;
  color = str->font->color;
  if (context->really_focus)
    {
      if(str->really_focused_color)
        color = str->really_focused_color;
      else if(str->font->really_focused_color)
        color = str->font->really_focused_color;
      else if (str->focused_color)
        color = str->focused_color;
      else if (str->font->focused_color)
        color = str->font->focused_color;
      else if (str->color)
        color = str->color;
    }
  else if (context->focus)
    {
      if (str->focused_color)
        color = str->focused_color;
      else if (str->font->focused_color)
        color = str->font->focused_color;
      else if (str->color)
        color = str->color;
    }
  else if (str->color)
    color = str->color;

  while ((c = mpui_string_get_next_char (&txt, str->string->encoding)))
    {
      if (c == '\n')
        {
          context->x = x_init;
          context->y += font->height;
          continue;
        }
      render_one_glyph (font, c);
      if ((f = font->font[c]) >= 0
          && (context->x + font->width[c] <= mpi->w)
          && (context->x + font->width[c] + font->charspace <= x_end)
          && (context->y + font->pic_a[f]->h <= mpi->h))
        mpui_render_glyph (font, c, context->x, context->y, color, mpi);
      context->x += font->width[c] + font->charspace;
    }
}

static inline void
mpui_render_element (mpui_element_t *element, mp_image_t *mpi,
                     mpui_render_context_t context);

static inline void
mpui_render_allmenuitem (mpui_allmenuitem_t *allmenuitem, mp_image_t *mpi,
                         mpui_render_context_t context)
{
  mpui_element_t **melem, **elements, *element = (mpui_element_t *)allmenuitem;

  for (melem=allmenuitem->menu->elements; *melem; melem++)
    {
      if ((*melem)->type == MPUI_MENUITEM)
        {
          context.focus = context.really_focus = 0;
          if ((*melem)->flags & MPUI_FLAG_FOCUSABLE)
            if (mpui_is_focused (context.screen, *melem))
              {
                context.focus = 1;
                if (mpui_is_really_focused (context.screen, *melem))
                  context.really_focus = 1;
              }

          element->x = (*melem)->x;
          element->y = (*melem)->y;
          element->w = (*melem)->w;
          element->h = (*melem)->h;

          mpui_recompute_coord (context.mpui, element,
                                element->w.val, element->h.val,
                                1, context.focus, context.really_focus);
          mpui_clip (context.mpui, element, context.x, context.y, 1);

          mpui_render_context_update (element, &context, 0);

          for (elements = ((mpui_container_t *) allmenuitem)->elements;
               *elements; elements++)
            mpui_render_element (*elements, mpi, context);

          mpui_render_context_update (element, &context, 1);
        }
    }
}

static inline void
mpui_render_element (mpui_element_t *element, mp_image_t *mpi,
                     mpui_render_context_t context)
{
  if (!element || element->flags & MPUI_FLAG_HIDDEN)
    return;

  if (element->flags & MPUI_FLAG_FOCUSABLE)
    {
      if (mpui_is_focused (context.screen, element))
        {
          context.focus = 1;
          if (mpui_is_really_focused (context.screen, element))
            context.really_focus = 1;
        }
    }

  if (element->when_focused == MPUI_DISPLAY_ALWAYS
      || (context.focus && element->when_focused == MPUI_DISPLAY_FOCUSED)
      || (!context.really_focus && element->when_focused==MPUI_DISPLAY_NORMAL)
      || (!context.focus && element->when_focused==MPUI_DISPLAY_REALLY_NORMAL)
      || (context.really_focus
          && element->when_focused == MPUI_DISPLAY_REALLY_FOCUSED))
    {
      if (element->type == MPUI_ALLMENUITEM)
        {
          mpui_render_allmenuitem ((mpui_allmenuitem_t *)element, mpi,context);
          return;
        }

      mpui_render_context_update (element, &context, 0);

      if (element->flags & MPUI_FLAG_CONTAINER)
        {
          mpui_element_t **elements;
          if (element->type == MPUI_MNU
              && ((mpui_mnu_t *) element)->menu->is_browser)
            mpui_browser_update (context.mpui, (mpui_mnu_t *) element);
          else if (element->type == MPUI_MNU
              && ((mpui_mnu_t *) element)->menu->is_playlist)
            mpui_playlist_update ((mpui_mnu_t *) element);
          else if (element->type == MPUI_INF)
            mpui_info_update (context.mpui, (mpui_inf_t *) element);
          else if (element->type == MPUI_SLIDESHOW)
            mpui_slideshow_update (context.mpui, (mpui_slideshow_t *) element,
                                   context.x, context.y);
          for (elements = ((mpui_container_t *) element)->elements;
               *elements; elements++)
            {
              if ((*elements)->type == MPUI_MENUITEM
                  || (*elements)->type == MPUI_ALLMENUITEM)
                {
                  mpui_focus_box_t *focus_box = (mpui_focus_box_t *) element;
                  if ((*elements)->type == MPUI_MENUITEM)
                    {
                      if (focus_box->orientation & MPUI_ORIENTATION_V)
                        {
                          if ((*elements)->y.val - focus_box->yoffset < 0)
                            continue;
                          if ((*elements)->y.val + (*elements)->h.val
                              - focus_box->yoffset > element->h.val)
                            continue;
                        }
                      if (focus_box->orientation & MPUI_ORIENTATION_H)
                        {
                          if ((*elements)->x.val - focus_box->xoffset < 0)
                            continue;
                          if ((*elements)->x.val + (*elements)->w.val
                              - focus_box->xoffset > element->w.val)
                            continue;
                        }
                    }
                  mpui_render_context_update_offset (focus_box, &context, 0);
                  mpui_render_element (*elements, mpi, context);
                  mpui_render_context_update_offset (focus_box, &context, 1);
                }
              else
                mpui_render_element (*elements, mpi, context);
            }
        }
      else
        switch (element->type)
          {
          case MPUI_IMG:
            mpui_render_image ((mpui_img_t *) element, mpi, &context);
            break;
          case MPUI_STR:
            mpui_render_string ((mpui_str_t *) element, mpi, &context);
            break;
          default:
            break;
          }
    }
}

int
mpui_render_screen (mpui_t *mpui, mp_image_t *mpi)
{
  mpui_render_context_t context;
  mpui_element_t **elements;

  if (!mpui->current_screen)
    return 1;

  context.mpui = mpui;
  context.screen = mpui->current_screen;
  context.focus = 0;
  context.really_focus = 0;
  context.x = 0;
  context.y = 0;

  for (elements=context.screen->elements; *elements; elements++)
    mpui_render_element (*elements, mpi, context);

  for (elements=(mpui_element_t **)context.screen->popup_stack;
       *elements; elements++)
    mpui_render_element (*elements, mpi, context);

  return 0;
}
