/*
 *  mpui_info.c: libmpui informations on files.
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "libmpdemux/stream.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"

#include "mpui_struct.h"
#include "mpui_info.h"

#define MPUI_INFO_CODEC "Codec"
#define MPUI_INFO_TITLE "Title"
#define MPUI_INFO_ARTIST "Artist"
#define MPUI_INFO_ALBUM "Album"
#define MPUI_INFO_YEAR "Year"
#define MPUI_INFO_COMMENT "Comment"
#define MPUI_INFO_GENRE "Genre"
#define MPUI_INFO_RESOLUTION "Resolution"
#define MPUI_INFO_DEPTH "Depth"
#define MPUI_INFO_FPS "Fps"
#define MPUI_INFO_VBITRATE "VBitrate"
#define MPUI_INFO_ABITRATE "ABitrate"
#define MPUI_INFO_SIZE "Size"

static char *
mpui_tag_update (demuxer_t *demuxer, char *file, mpui_tag_t *tag)
{
  sh_video_t *video = NULL;
  sh_audio_t *audio = NULL;
  struct stat st;
  static char tmp[128];
  char *info;

  if (demuxer->video)
    video = (sh_video_t *) demuxer->video->sh;
  if (demuxer->audio)
    audio = (sh_audio_t *) demuxer->audio->sh;

  if (!strcmp (tag->type, MPUI_INFO_SIZE))
    {
      if (stat (file, &st) == 0)
        snprintf (tmp, 128, "%.2f MB", (float) st.st_size / 1024 / 1024);
    }
  else if (!strcmp (tag->type, MPUI_INFO_CODEC))
    {
      if (video && video->bih)
        return ((char *) &video->bih->biCompression);
      else
        return (info = demux_info_get (demuxer, MPUI_INFO_CODEC)) ? info : "";
    }
  else if (video && video->bih && !strcmp (tag->type, MPUI_INFO_RESOLUTION))
    snprintf (tmp, 128, "%d x %d",
              video->bih->biWidth, video->bih->biHeight);
  else if (video && video->bih && !strcmp (tag->type, MPUI_INFO_DEPTH))
    snprintf (tmp, 128, "%d bpp", video->bih->biBitCount);
  else if (video && !strcmp (tag->type, MPUI_INFO_FPS))
    snprintf (tmp, 128, "%5.3f fps", video->fps);
  else if (video && !strcmp (tag->type, MPUI_INFO_VBITRATE))
    snprintf (tmp, 128, "%5.1f kbps", video->i_bps * 0.008f);
  else if (audio && !strcmp (tag->type, MPUI_INFO_ABITRATE))
    snprintf (tmp, 128, "%5.1f kbps", audio->i_bps * 0.008f);
  else
    return (info = demux_info_get (demuxer, tag->type)) ? info : "";

  return tmp;
}

static void
mpui_info_add_tag (mpui_inf_t *inf, mpui_tag_t *tag,
                   mpui_coord_t *cx, mpui_coord_t *cy,
                   demuxer_t *demuxer, char *file)
{
  mpui_string_t *string;
  mpui_str_t *str;
  mpui_coord_t x, y;
  char tmp[256];

  x = (tag->x.val) ? tag->x : *cx;
  y = (tag->y.val) ? tag->y : *cy;

  snprintf (tmp, 128, "%s : %s", tag->caption,
            mpui_tag_update (demuxer, file, tag));

  string = mpui_string_new (NULL, tmp, MPUI_ENCODING_UTF8);
  str = mpui_str_new (string, x, y, 0, inf->info->font,
                      MPUI_FONT_SIZE_DEFAULT, NULL, NULL,
                      NULL, MPUI_DISPLAY_ALWAYS);

  mpui_container_elements_add ((mpui_container_t *) inf, 
                               (mpui_element_t *) str);

  *cx = x;
  (*cy).val = y.val + ((mpui_element_t *) str)->h.val;
}

void
mpui_info_clean (mpui_inf_t *inf)
{
  mpui_element_t **elements;
  for (elements = ((mpui_container_t *) inf)->elements; *elements; elements++)
    if (*elements)
      mpui_element_free (*elements);
  *((mpui_container_t *) inf)->elements = NULL;
}

void
mpui_info_update (mpui_inf_t *inf, char *filename)
{
  mpui_tag_t **tag;
  stream_t* stream = NULL;
  demuxer_t *demuxer = NULL;
  mpui_coord_t x, y;
  int file_format;

  mpui_info_clean (inf);
  if (!filename)
    return;

  stream = open_stream (filename, 0, &file_format);
  if (!stream)
    return;

  demuxer = demux_open (stream, file_format, -1, -1, -1, filename);
  if (!demuxer)
    {
      free_stream (stream);
      return;
    }

  x = inf->info->x;
  y = inf->info->y;

  for (tag = inf->info->tags; *tag; tag++)
    mpui_info_add_tag (inf, *tag, &x, &y, demuxer, filename);

  free_demuxer (demuxer);
  free_stream (stream);
}
