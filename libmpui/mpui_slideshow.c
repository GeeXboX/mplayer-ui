/*
 *  mpui_slideshow.c: libmpui slideshow management.
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

#include <unistd.h>
#include <dirent.h>

#include "osdep/timer.h"

#include "mpui_struct.h"
#include "mpui_image.h"
#include "mpui_parser.h"
#include "mpui_slideshow.h"


static void
mpui_slideshow_generate (mpui_t *mpui, mpui_slideshow_t *slideshow,
                         mpui_size_t context_x, mpui_size_t context_y)
{
  char *name, filename[NAME_MAX];
  mpui_image_t *image;
  mpui_img_t *img;
  mpui_coord_t x, y, z;

  z.val = 0;
  x.str = y.str = z.str = NULL;

  name = slideshow->dirent[slideshow->dirent_cur]->d_name;
  snprintf (filename, sizeof (filename), "%s/%s", slideshow->path, name);
  image = mpui_image_new (NULL, filename, 0, 0, 0, 0);
  mpui_image_load (image, slideshow->rotation);

  if (image->h > ((mpui_element_t *) slideshow)->h.val
      || slideshow->mode == MPUI_SLIDESHOW_MODE_ZOOM)
    {
      image->w = image->w * ((mpui_element_t *)slideshow)->h.val /image->h;
      image->h = ((mpui_element_t *) slideshow)->h.val;
    }
  if (image->w > ((mpui_element_t *) slideshow)->w.val)
    {
      image->h = image->h * ((mpui_element_t *)slideshow)->w.val /image->w;
      image->w = ((mpui_element_t *) slideshow)->w.val;
    }
  x.val = (((mpui_element_t *) slideshow)->w.val - image->w) / 2;
  y.val = (((mpui_element_t *) slideshow)->h.val - image->h) / 2;

  img = mpui_img_new (image, x, y, z, z, 0, MPUI_DISPLAY_ALWAYS);
  mpui_img_load (mpui, img);
  mpui_clip (mpui, (mpui_element_t *) img, 0, 0, 0);
  mpui_container_elements_add ((mpui_container_t *) slideshow,
                               (mpui_element_t *) img);

  if (slideshow->border)
    {
      mpui_element_t *element = (mpui_element_t *) slideshow->border;
      element->x = x;
      element->y = y;
      element->w = ((mpui_element_t *) img)->w;
      element->h = ((mpui_element_t *) img)->h;
      mpui_recompute_coord (mpui, element, element->w.val, element->h.val,
                            1, 0, 0);
      mpui_clip (mpui, element, context_x, context_y, 1);
      mpui_container_elements_add ((mpui_container_t *) slideshow, element);
    }

  if (slideshow->name_font)
    {
      mpui_string_t *string;
      mpui_str_t *str;

      string = mpui_string_new (NULL, name, MPUI_ENCODING_UTF8);
      str = mpui_str_new (string, slideshow->name_x, slideshow->name_y, 0,
                          slideshow->name_font, MPUI_FONT_SIZE_DEFAULT,
                          NULL, NULL, NULL, MPUI_DISPLAY_ALWAYS);
      if (slideshow->name_x.val == -1)
        {
          if (((mpui_element_t *) str)->w.val
              > ((mpui_element_t *) slideshow)->w.val)
            ((mpui_element_t *) str)->x.val = 0;
          else
            ((mpui_element_t *) str)->x.val
              = (((mpui_element_t *) slideshow)->w.val
                 - ((mpui_element_t *) str)->w.val) / 2;
        }
      mpui_container_elements_add ((mpui_container_t *) slideshow,
                               (mpui_element_t *) str);
    }

  slideshow->need_generate = 0;
}

void
mpui_slideshow_cleanup (mpui_slideshow_t *slideshow)
{
  mpui_element_t **elements;

  for (elements=((mpui_container_t *) slideshow)->elements;
       *elements; elements++)
    if (*elements != (mpui_element_t *) slideshow->border)
      {
        if ((*elements)->type == MPUI_IMG)
          mpui_image_free (((mpui_img_t *) *elements)->image);
        mpui_element_free (*elements);
      }
  *((mpui_container_t *) slideshow)->elements = NULL;
}


void
mpui_slideshow_path (mpui_slideshow_t *slideshow, char *path)
{
  if (path)
    {
      char *p;
      strncpy (slideshow->path, path, sizeof (slideshow->path) - 1);
      free (slideshow->name);
      if (strlen (slideshow->path) > 1 && (p = strrchr (slideshow->path, '/')))
        {
          *p = '\0';
          slideshow->name = strdup (p+1);
        }
      else
        slideshow->name = NULL;
    }
  else
    getcwd (slideshow->path, sizeof (slideshow->path));
  slideshow->path_id++;
}

void
mpui_slideshow_pause (mpui_slideshow_t *slideshow)
{
  if (slideshow->timer)
    slideshow->play = !slideshow->play;
}

static int
mpui_slideshow_verify_filetype (mpui_filetypes_t *filter, char *filename)
{
  mpui_filetype_t **filetypes;
  char **exts;
  int len, l;

  len = strlen (filename);

  for (filetypes=filter->filetypes; *filetypes; filetypes++)
    switch ((*filetypes)->match)
      {
      case MPUI_MATCH_EXT:
        for (exts=(*filetypes)->exts; *exts; exts++)
          {
            l = strlen (*exts) + 1;
            if (len >= l && filename[len-l] == '.'
                && !strcasecmp (*exts, filename+len-l+1))
              return 1;
          }
        break;
      case MPUI_MATCH_ALL:
        return 1;
      default:
        break;
      }

  return 0;
}

static int
mpui_slideshow_find_name (mpui_slideshow_t *slideshow)
{
  int i;

  if (slideshow->name)
    for (i=0; i<slideshow->dirent_size; i++)
      if (!strcmp(slideshow->dirent[i]->d_name, slideshow->name)
          && mpui_slideshow_verify_filetype (slideshow->filter,
                                             slideshow->dirent[i]->d_name))
        {
          slideshow->dirent_cur = i;
          slideshow->rotation = 0;
          slideshow->need_generate = 1;
          return 0;
        }

  return 1;
}

void
mpui_slideshow_prev (mpui_slideshow_t *slideshow)
{
  int i;

  for (i=slideshow->dirent_cur-1; i>=0; i--)
    if (mpui_slideshow_verify_filetype (slideshow->filter,
                                        slideshow->dirent[i]->d_name))
      break;
  if (i < 0)
    {
      for (i=slideshow->dirent_size-1; i>slideshow->dirent_cur; i--)
        if (mpui_slideshow_verify_filetype (slideshow->filter,
                                            slideshow->dirent[i]->d_name))
          break;
      if (i <= slideshow->dirent_cur)
        return;
    }

  slideshow->dirent_cur = i;
  slideshow->rotation = 0;
  slideshow->need_generate = 1;
}

void
mpui_slideshow_next (mpui_slideshow_t *slideshow)
{
  int i;

  for (i=slideshow->dirent_cur+1; i<slideshow->dirent_size; i++)
    if (mpui_slideshow_verify_filetype (slideshow->filter,
                                        slideshow->dirent[i]->d_name))
      break;
  if (i >= slideshow->dirent_size)
    {
      for (i=0; i<slideshow->dirent_cur; i++)
        if (mpui_slideshow_verify_filetype (slideshow->filter,
                                            slideshow->dirent[i]->d_name))
          break;
      if (i >= slideshow->dirent_cur)
        return;
    }

  slideshow->dirent_cur = i;
  slideshow->rotation = 0;
  slideshow->need_generate = 1;
}

void
mpui_slideshow_mode (mpui_slideshow_t *slideshow, char *mode)
{
  if (mode)
    {
      if (!strcmp (mode, "zoom"))
        slideshow->mode = MPUI_SLIDESHOW_MODE_ZOOM;
      else
        slideshow->mode = MPUI_SLIDESHOW_MODE_1_1;
    }
  else
    {
      if (slideshow->mode == MPUI_SLIDESHOW_MODE_1_1)
        slideshow->mode = MPUI_SLIDESHOW_MODE_ZOOM;
      else
        slideshow->mode = MPUI_SLIDESHOW_MODE_1_1;
    }

  slideshow->need_generate = 1;
}

void
mpui_slideshow_rotate (mpui_slideshow_t *slideshow, int rotate)
{
  slideshow->rotation = (slideshow->rotation + rotate + 256) % 4;
  slideshow->need_generate = 1;
}

void
mpui_slideshow_update (mpui_t *mpui, mpui_slideshow_t *slideshow,
                       mpui_size_t context_x, mpui_size_t context_y)
{
  if (slideshow->last_path_id != slideshow->path_id)
    {
      if (slideshow->dirent && slideshow->dirent_size > 0)
        {
          while (slideshow->dirent_size--)
            free (slideshow->dirent[slideshow->dirent_size]);
          free (slideshow->dirent);
        }

      slideshow->dirent_size = scandir (slideshow->path, &slideshow->dirent,
                                        NULL, alphasort);
      slideshow->dirent_cur = -1;

      slideshow->last_path_id = slideshow->path_id;

      if (mpui_slideshow_find_name (slideshow))
        mpui_slideshow_next (slideshow);
    }
  else if (slideshow->play && GetTimer() > slideshow->next_timer)
    mpui_slideshow_next (slideshow);

  if (mpui && slideshow->need_generate && slideshow->dirent_cur >= 0)
    {
      mpui_slideshow_cleanup (slideshow);
      mpui_slideshow_generate (mpui, slideshow, context_x, context_y);
      slideshow->next_timer = GetTimer () + 1000*slideshow->timer;
    }
}
