/*
 *  mpui_image.c: libmpui image loader and converter.
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

#include <stdio.h>
#include <string.h>
#include <png.h>
#include <jpeglib.h>
#include <setjmp.h>

#include <stdint.h>
#include "postproc/swscale.h"
#include "libmpcodecs/img_format.h"

#include "mpui_image.h"


static void
mpui_image_planar_permultiply_alpha (mpui_image_t *image)
{
  unsigned char *y, *u, *v, *ay, *auv, *end;

  y   = image->planes[0];
  u   = image->planes[1];
  v   = image->planes[2];
  ay  = image->planes[3];
  auv = image->planes[4];

  for (end=y+image->stride[0]*image->h; y<end; y++, ay++)
    {
      *y = (*y * (*ay + 1)) >> 8;
      *ay = 255 - *ay;
    }
  for (end=u+image->stride[1]*image->chroma_height; u<end; u++, v++, auv++)
    {
      *u = (*u * (*auv + 1)) >> 8;
      *v = (*v * (*auv + 1)) >> 8;
      *auv = 255 - *auv;
    }
}

static void
mpui_image_packed_permultiply_alpha (mpui_image_t *image)
{
  unsigned char *y, *ay, *end;

  y   = image->planes[0];
  ay  = image->planes[3];

  for (end=y+image->stride[0]*image->h; y<end; )
    {
      *y = (*y * (*ay + 1)) >> 8;    y++;
      *y = (*y * (*ay + 1)) >> 8;    y++;
      *y = (*y * (*(ay+1)+1)) >> 8;  y++;
      *y = (*y * (*ay + 1)) >> 8;    y++;
      *ay = 255 - *ay;              ay++;
      *ay = 255 - *ay;              ay++;
    }
}

static void
mpui_image_permultiply_alpha (mpui_image_t *image, int num_planes)
{
  if (num_planes > 1)
    mpui_image_planar_permultiply_alpha (image);
  else
    mpui_image_packed_permultiply_alpha (image);
}

void
mpui_image_convert (mpui_image_t *image, mpui_raw_image_t *raw, int format)
{
  struct SwsContext *sws;
  float dbpp;
  int num_planes = 0;
  unsigned char *src[3];
  int src_stride[3];
  int sformat;

  image->alpha = raw->alpha;
  sformat = raw->format;
  dbpp = image->alpha ? 2.75 : 1.5;

  switch (format)
    {
    case IMGFMT_YVU9:
      num_planes = 3;
      image->chroma_width = image->w >> 2;
      image->chroma_height = image->h >> 2;
      dbpp = image->alpha ? 2.1875 : 1.125;
      break;
    case IMGFMT_I420:
    case IMGFMT_IYUV:
      sformat = sformat == IMGFMT_BGR24 ? IMGFMT_RGB24 : IMGFMT_RGB32;
    case IMGFMT_YV12:
      num_planes = 3;
      image->chroma_width = image->w >> 1;
      image->chroma_height = image->h >> 1;
      dbpp = image->alpha ? 2.75 : 1.5;
      break;
    case IMGFMT_UYVY:
    case IMGFMT_YUY2:
      num_planes = 1;
      image->bpp = 2;
      dbpp = image->alpha ? image->bpp + 1 : image->bpp;
      /* strange but needed to avoid color inversion */
      sformat = sformat == IMGFMT_BGR24 ? IMGFMT_RGB24 : IMGFMT_RGB32;
      break;
    }

  src[0] = raw->data;
  src_stride[0] = raw->stride;

  image->planes[0] = (unsigned char *) malloc(dbpp * image->w * image->h);
  if (num_planes >= 3)
    {
      image->planes[1] = image->planes[0] + image->w * image->h;
      image->planes[2] = image->planes[1] + image->chroma_width*image->chroma_height;
      image->stride[0] = image->w;
      image->stride[1] = image->chroma_width;
      image->stride[2] = image->chroma_width;
    }
  else
    image->stride[0] = image->bpp * image->w;

  sws = sws_getContext (raw->w, raw->h, sformat, image->w,
                        image->h, format, SWS_BILINEAR, NULL, NULL, NULL);
  if (!sws)
    {
      free (image->planes[0]);
      return;
    }
  sws_scale (sws, src, src_stride, 0, raw->h,
             image->planes, image->stride);
  sws_freeContext (sws);
  image->format = format;

  if (image->alpha)
    {
      unsigned char *alpha_buffer, *a, *img, *end;

      if (num_planes >= 3)
        image->planes[3] = image->planes[2] + image->chroma_width*image->chroma_height;
      else
        image->planes[3] = image->planes[0] + image->bpp * image->w * image->h;
      image->stride[3] = image->w;

      img = src[0] + 3;
      a = alpha_buffer = (unsigned char *) malloc (raw->w * raw->h);
      for (end=a+raw->w*raw->h; a<end; a++, img+=raw->bpp)
        *a = *img;
      src[0] = alpha_buffer;
      src_stride[0] = raw->w;

      sws = sws_getContext (raw->w, raw->h, IMGFMT_Y8,
                            image->w, image->h, IMGFMT_Y8,
                            SWS_BILINEAR, NULL, NULL, NULL);
      if (!sws)
        {
          free (image->planes[0]);
          free (alpha_buffer);
          return;
        }
      sws_scale (sws, src, src_stride, 0, raw->h,
                 image->planes+3, image->stride+3);
      sws_freeContext (sws);

      if (num_planes >= 3)
        {
          image->planes[4] = image->planes[3] + image->w * image->h;
          image->stride[4] = image->chroma_width;

          sws = sws_getContext (raw->w, raw->h, IMGFMT_Y8,
                                image->chroma_width, image->chroma_height,
                                IMGFMT_Y8, SWS_BILINEAR, NULL, NULL, NULL);
          if (!sws)
            {
              free (image->planes[0]);
              free (alpha_buffer);
              return;
            }
          sws_scale (sws, src, src_stride, 0, raw->h,
                     image->planes+4, image->stride+4);
          sws_freeContext (sws);
        }

      free (alpha_buffer);

      mpui_image_permultiply_alpha (image, num_planes);
    }

  image->num_planes = num_planes;
}


static int
mpui_image_load_png (mpui_raw_image_t *raw, FILE *fp)
{
  png_structp png;
  png_infop info, end_info;
  png_uint_32 width, height;
  png_bytep *row;
  int depth, color;
  unsigned int i;

  png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info = png_create_info_struct (png);
  end_info = png_create_info_struct (png);

  png_init_io (png, fp);
  png_read_info (png, info);
  png_get_IHDR (png, info, &width, &height, &depth, &color, NULL, NULL, NULL);
  if (png_get_valid (png, info, PNG_INFO_tRNS))
    raw->alpha = 1;
  else
    raw->alpha = color & PNG_COLOR_MASK_ALPHA;
  raw->format = raw->alpha ? IMGFMT_BGR32 : IMGFMT_BGR24;
  raw->w = width;
  raw->h = height;
  raw->bpp = raw->alpha ? 4 : 3;
  raw->stride = raw->bpp * raw->w;

  png_set_expand (png);          /* expand to 24 bits RGB */
  png_set_palette_to_rgb (png);  /* expand 8 bits paletted to RGB */
  png_set_gray_to_rgb (png);     /* expand greyscale to RGB */
  png_set_strip_16(png);         /* convert 16 bits per channel to 8 bits */
  png_set_tRNS_to_alpha(png);    /* expand tRNS to a full alpha channel */
#if 0  /* add an opaque aplha channel (may help to optimize ?) */
  png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
#endif

  raw->data = (unsigned char *) malloc (height * raw->stride);
  row = (png_bytep *) malloc (sizeof (*row) * height);
  for (i=0; i < height; i++)
    row[i] = raw->data + raw->stride*i;
  png_read_image (png, row);
  free (row);

  png_read_end (png, end_info);
  png_destroy_read_struct (&png, &info, &end_info);

  return 0;
}

static jmp_buf jmp_env;

static void
mpui_image_jpeg_error (j_common_ptr cinfo)
{
  longjmp (jmp_env, 1);
}

static int
mpui_image_load_jpeg (mpui_raw_image_t *raw, FILE *fp)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPLE **row = NULL;
  unsigned int i;
  int ret = 1;

  cinfo.err = jpeg_std_error (&jerr);
  cinfo.err->error_exit = mpui_image_jpeg_error;
  if (!setjmp (jmp_env))
    {
      jpeg_create_decompress (&cinfo);
      jpeg_stdio_src (&cinfo, fp);
      jpeg_read_header (&cinfo, TRUE);
      cinfo.out_color_space = JCS_RGB;
      jpeg_start_decompress (&cinfo);
      raw->alpha = 0;
      raw->format = IMGFMT_BGR24;
      raw->w = cinfo.output_width;
      raw->h = cinfo.output_height;
      raw->bpp = 3;
      raw->stride = 3 * raw->w;
      raw->data = (unsigned char *) malloc (3 * cinfo.output_width
                                              * cinfo.output_height);
      row = (JSAMPLE **) malloc (sizeof (*row) * cinfo.output_height);
      for (i=0; i < cinfo.output_height; i++)
        row[i] = raw->data + 3*i*cinfo.output_width;
      while (cinfo.output_scanline < cinfo.output_height)
        jpeg_read_scanlines (&cinfo, row + cinfo.output_scanline,
                             cinfo.output_height - cinfo.output_scanline);
      jpeg_finish_decompress (&cinfo);
      ret = 0;
    }
  else
    {
      free (raw->data);
      raw->data = NULL;
    }
  free (row);
  jpeg_destroy_decompress (&cinfo);
  return ret;
}

int
mpui_image_load (mpui_image_t *image)
{
  png_byte header[8];
  int ret = 1;
  FILE *fp;

  fp = fopen (image->file, "rb");
  if (!fp)
    return ret;
  fread (header, 1, sizeof (header), fp);
  fseek (fp, 0, SEEK_SET);
  if (png_check_sig (header, sizeof (header)))
    ret = mpui_image_load_png (&image->raw, fp);
  else if (!memcmp(header, "\xFF\xD8\xFF\xE0\x00\x10\x4A\x46", sizeof(header)))
    ret = mpui_image_load_jpeg (&image->raw, fp);
  fclose (fp);

  if (!ret)
    {
      if (image->w == 0 && image->h == 0)
        {
          image->w = image->raw.w;
          image->h = image->raw.h;
        }
      else
        {
          if (image->w == 0)
            image->w = image->h*image->raw.w/image->raw.h;
          if (image->h == 0)
            image->h = image->w*image->raw.h/image->raw.w;
        }
    }

  return ret;
}
