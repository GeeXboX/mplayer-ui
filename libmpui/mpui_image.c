/*
 *  mpui_image.c: libmpui image loader and converter.
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

#include <stdio.h>
#include <string.h>
#include <png.h>
#include <jpeglib.h>
#include <setjmp.h>

#include <stdint.h>
#include "../postproc/swscale.h"
#include "../libmpcodecs/img_format.h"

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
mpui_image_permultiply_alpha (mpui_image_t *image)
{
  if (image->num_planes > 1)
    mpui_image_planar_permultiply_alpha (image);
  else
    mpui_image_packed_permultiply_alpha (image);
}

static void
mpui_image_convert (mpui_image_t *image, int format)
{
  struct SwsContext *sws;
  int sbpp = image->bpp;
  float dbpp = image->alpha ? 2.75 : 1.5;
  unsigned char *src[3];
  int src_stride[3];

  switch (format)
    {
    case IMGFMT_YV12:
      image->num_planes = 3;
      image->chroma_width = image->w >> 1;
      image->chroma_height = image->h >> 1;
      dbpp = image->alpha ? 2.75 : 1.5;
      break;
    case IMGFMT_YUY2:
      image->num_planes = 1;
      image->bpp = 2;
      dbpp = image->alpha ? image->bpp + 1 : image->bpp;
      /* strange but needed to avoid color inversion */
      image->format = image->format == IMGFMT_BGR24 ? IMGFMT_RGB24 : IMGFMT_RGB32;
      break;
    }

  src[0] = image->planes[0];
  src_stride[0] = image->stride[0];

  image->planes[0] = (unsigned char *) malloc(dbpp * image->w * image->h);
  if (image->num_planes >= 3)
    {
      image->planes[1] = image->planes[0] + image->w * image->h;
      image->planes[2] = image->planes[1] + image->chroma_width*image->chroma_height;
      image->stride[0] = image->w;
      image->stride[1] = image->chroma_width;
      image->stride[2] = image->chroma_width;
    }
  else
    image->stride[0] = image->bpp * image->w;

  sws = sws_getContext (image->width, image->height, image->format, image->w,
                        image->h, format, SWS_BILINEAR, NULL, NULL, NULL);
  sws_scale (sws, src, src_stride, 0, image->height,
             image->planes, image->stride);
  sws_freeContext (sws);
  image->format = format;

  if (image->alpha)
    {
      unsigned char *alpha_buffer, *a, *img, *end;

      if (image->num_planes >= 3)
        image->planes[3] = image->planes[2] + image->chroma_width*image->chroma_height;
      else
        image->planes[3] = image->planes[0] + image->bpp * image->w * image->h;
      image->stride[3] = image->w;

      img = src[0] + 3;
      a = alpha_buffer = (unsigned char *) malloc (image->width*image->height);
      for (end=a+image->width*image->height; a<end; a++, img+=sbpp)
        *a = *img;
      free (src[0]);
      src[0] = alpha_buffer;
      src_stride[0] = image->width;

      sws = sws_getContext (image->width, image->height, IMGFMT_Y8,
                            image->w, image->h, IMGFMT_Y8,
                            SWS_BILINEAR, NULL, NULL, NULL);
      sws_scale (sws, src, src_stride, 0, image->height,
                 image->planes+3, image->stride+3);
      sws_freeContext (sws);

      if (image->num_planes >= 3)
        {
          image->planes[4] = image->planes[3] + image->w * image->h;
          image->stride[4] = image->chroma_width;

          sws = sws_getContext (image->width, image->height, IMGFMT_Y8,
                                image->chroma_width, image->chroma_height,
                                IMGFMT_Y8, SWS_BILINEAR, NULL, NULL, NULL);
          sws_scale (sws, src, src_stride, 0, image->height,
                     image->planes+4, image->stride+4);
          sws_freeContext (sws);
        }

      mpui_image_permultiply_alpha (image);
    }

  free (src[0]);
}


static int
mpui_image_load_png (mpui_image_t *image, FILE *fp)
{
  png_structp png;
  png_infop info, end_info;
  png_uint_32 width, height;
  png_bytep *row;
  int depth, color;
  int i, line_length;

  png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info = png_create_info_struct (png);
  end_info = png_create_info_struct (png);

  png_init_io (png, fp);
  png_read_info (png, info);
  png_get_IHDR (png, info, &width, &height, &depth, &color, NULL, NULL, NULL);
  if (png_get_valid (png, info, PNG_INFO_tRNS))
    image->alpha = 1;
  else
    image->alpha = color & PNG_COLOR_MASK_ALPHA;
  image->format = image->alpha ? IMGFMT_BGR32 : IMGFMT_BGR24;
  image->width = width;
  image->height = height;
  image->bpp = image->alpha ? 4 : 3;
  image->stride[0] = image->bpp * image->width;

  png_set_expand (png);          /* expand to 24 bits RGB */
  png_set_palette_to_rgb (png);  /* expand 8 bits paletted to RGB */
  png_set_gray_to_rgb (png);     /* expand greyscale to RGB */
  png_set_strip_16(png);         /* convert 16 bits per channel to 8 bits */
  png_set_tRNS_to_alpha(png);    /* expand tRNS to a full alpha channel */
#if 0  /* add an opaque aplha channel (may help to optimize ?) */
  png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
#endif

  line_length = width * (image->alpha ? 4 : 3);
  image->planes[0] = (unsigned char *) malloc (height * line_length);
  image->num_planes = 1;
  row = (png_bytep *) malloc (sizeof (*row) * height);
  for (i=0; i < height; i++)
    row[i] = image->planes[0] + line_length*i;
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
mpui_image_load_jpeg (mpui_image_t *image, FILE *fp)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPLE **row = NULL;
  int i, ret = 1;

  cinfo.err = jpeg_std_error (&jerr);
  cinfo.err->error_exit = mpui_image_jpeg_error;
  if (!setjmp (jmp_env))
    {
      jpeg_create_decompress (&cinfo);
      jpeg_stdio_src (&cinfo, fp);
      jpeg_read_header (&cinfo, TRUE);
      cinfo.out_color_space = JCS_RGB;
      jpeg_start_decompress (&cinfo);
      image->format = IMGFMT_BGR24;
      image->width = cinfo.output_width;
      image->height = cinfo.output_height;
      image->bpp = 3;
      image->stride[0] = 3 * image->width;
      image->planes[0] = (unsigned char *) malloc (3 * cinfo.output_width
                                                     * cinfo.output_height);
      image->num_planes = 1;
      row = (JSAMPLE **) malloc (sizeof (*row) * cinfo.output_height);
      for (i=0; i < cinfo.output_height; i++)
        row[i] = image->planes[0] + 3*i*cinfo.output_width;
      while (cinfo.output_scanline < cinfo.output_height)
        jpeg_read_scanlines (&cinfo, row + cinfo.output_scanline,
                             cinfo.output_height - cinfo.output_scanline);
      jpeg_finish_decompress (&cinfo);
      ret = 0;
    }
  free (row);
  jpeg_destroy_decompress (&cinfo);
  return ret;
}

int
mpui_image_load (mpui_image_t *image, char *filename, int format)
{
  png_byte header[8];
  int ret = 1;
  FILE *fp;

  fp = fopen (filename, "rb");
  if (!fp)
    return ret;
  fread (header, 1, sizeof (header), fp);
  fseek (fp, 0, SEEK_SET);
  if (png_check_sig (header, sizeof (header)))
    ret = mpui_image_load_png (image, fp);
  else if (!memcmp(header, "\xFF\xD8\xFF\xE0\x00\x10\x4A\x46", sizeof(header)))
    ret = mpui_image_load_jpeg (image, fp);
  fclose (fp);

  if (!ret)
    mpui_image_convert (image, format);

  return ret;
}
