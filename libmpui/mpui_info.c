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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include "libmpdemux/stream.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"
#include "osdep/timer.h"
#include "version.h"

#include "mpui_struct.h"
#include "mpui_parser.h"
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

#define MPUI_SYSINFO_DATE "Date"
#define MPUI_SYSINFO_TIME "Time"
#define MPUI_SYSINFO_MEMORY "Memory"
#define MPUI_SYSINFO_NETWORK "Network"
#define MPUI_SYSINFO_BUILD "Build"

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
mpui_info_update_tag (mpui_inf_t *inf, mpui_tag_t *tag,
                      mpui_coord_t *cx, mpui_coord_t *cy, demuxer_t *demuxer)
{
  mpui_string_t *string;
  mpui_str_t *str;
  mpui_coord_t x, y;
  char tmp[256];

  x = (tag->x.val) ? tag->x : *cx;
  y = (tag->y.val) ? tag->y : *cy;

  snprintf (tmp, 128, "%s : %s", tag->caption,
            mpui_tag_update (demuxer, inf->filename, tag));

  string = mpui_string_new (NULL, tmp, MPUI_ENCODING_UTF8);
  str = mpui_str_new (string, x, y, 0, inf->info->font,
                      MPUI_FONT_SIZE_DEFAULT, NULL, NULL,
                      NULL, MPUI_DISPLAY_ALWAYS);

  mpui_container_elements_add ((mpui_container_t *) inf, 
                               (mpui_element_t *) str);

  *cx = x;
  (*cy).val = y.val + ((mpui_element_t *) str)->h.val;
}

static char *
mpui_info_get_picture_file (mpui_pic_t *pic, char *filename)
{
  mpui_filetype_t **filetypes;
  struct dirent **namelist;
  char *dir = filename, *cur, **exts, *res = NULL;
  int n, i, l, len;

  if ((filename = strrchr (filename, '/')))
    *filename++ = '\0';
  if ((cur = strrchr (filename, '.')))
    *cur = '\0';

  n = scandir (dir, &namelist, 0, alphasort);
  if (n < 0)
    return NULL;

  len = strlen (filename);

  for (i=0; i<n && !res; i++)
    if (!strncasecmp (filename, namelist[i]->d_name, len))
      {
        l = strlen (namelist[i]->d_name);
        cur = namelist[i]->d_name + l;

        for (filetypes=pic->filter->filetypes; *filetypes && !res; filetypes++)
          if ((*filetypes)->match == MPUI_MATCH_EXT)
            for (exts=(*filetypes)->exts; *exts; exts++)
              if (!strcasecmp (cur - strlen (*exts), *exts))
                {
                  res = (char *) malloc (strlen (dir) + l + 2);
                  sprintf (res, "%s/%s", dir, namelist[i]->d_name);
                  break;
                }
      }

  while (n--)
    free (namelist[n]);
  free (namelist);

  return res;
}

static void
mpui_info_update_pic (mpui_t *mpui, mpui_inf_t *inf, mpui_pic_t *pic)
{
  mpui_image_t *image = NULL;
  mpui_img_t *img = NULL;
  char *file;

  file = mpui_info_get_picture_file (pic, inf->filename);
 
  image = mpui_image_new (pic->id, file,
                          pic->x.val, pic->y.val, pic->w.val, pic->h.val);
  free (file);

  img = mpui_img_new (image, pic->x, pic->y, pic->w, pic->h,
                          0, MPUI_DISPLAY_ALWAYS);

  mpui_img_load (mpui, img);
  mpui_clip (mpui, (mpui_element_t *) img, 0, 0, 0);
  mpui_container_elements_add ((mpui_container_t *) inf,
                               (mpui_element_t *) img);
}

static char *
mpui_sys_update (mpui_sys_t *sys)
{
  static char tmp[128];

  bzero (tmp, 128);
  if (!strcmp (sys->type, MPUI_SYSINFO_DATE))
    {
      time_t t;
      struct tm *lt;

      t = time (NULL);
      lt = localtime (&t);
      sprintf (tmp, "%.2d / %.2d / %d",
               lt->tm_mon, lt->tm_mday, 1900 + lt->tm_year);
    }
  else if (!strcmp (sys->type, MPUI_SYSINFO_TIME))
    {
      time_t t;
      struct tm *lt;

      t = time (NULL);
      lt = localtime (&t);
      sprintf (tmp, "%.2d:%.2d:%.2d", lt->tm_hour, lt->tm_min, lt->tm_sec);
    }
  else if (!strcmp (sys->type, MPUI_SYSINFO_MEMORY))
    {
      FILE *fd;
      char line[128], *val;
      int active = 0, total = 0;

      fd = fopen ("/proc/meminfo", "r");
      if (!fd)
        return NULL;

      while (fgets (line, 128, fd))
        {
          if (!strncmp (line, "MemTotal", 8))
            {
              val = strtok (line, " ");
              val = strtok (NULL, " ");
              total = atoi (val);
            }
          if (!strncmp (line, "Active", 6))
            {
              val = strtok (line, " ");
              val = strtok (NULL, " ");
              active = atoi (val);
            }
        }
      fclose (fd);

      if (active && total)
        sprintf (tmp, "%d / %d MB (%d %% free)",
                 (int) (active / 1000), (int) (total / 1000),
                 (int) ((total - active) * 100 / total));
      else
        return NULL;
    }
  else if (!strcmp (sys->type, MPUI_SYSINFO_NETWORK))
    {
      struct if_nameindex *ifs, *ifp;
      struct ifreq ifr;
      struct in_addr addr;
      int fd;

      fd = socket (AF_INET, SOCK_DGRAM, 0);
      if (fd < 0)
        return NULL;

      for (ifp = ifs = if_nameindex (); ifp->if_index != 0; ifp++)
        {
          strcpy (ifr.ifr_name, ifp->if_name);
          ifr.ifr_addr.sa_family = AF_INET;

          if (ioctl (fd, SIOCGIFADDR, &ifr) == 0)
            {
              char *val = NULL;
              val = (char *) malloc (32);
              addr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;
              sprintf (val, "\n  %s : %s", ifp->if_name, inet_ntoa (addr));
              strcat (tmp, val);
              free (val);
            }
          else
            return NULL;
        }

      if_freenameindex (ifs);
      close (fd);
    }
  else if (!strcmp (sys->type, MPUI_SYSINFO_BUILD))
    return VERSION;

  return tmp;
}

static void
mpui_info_update_sys (mpui_inf_t *inf, mpui_sys_t *sys,
                      mpui_coord_t *cx, mpui_coord_t *cy)
{
  mpui_string_t *string;
  mpui_str_t *str;
  mpui_coord_t x, y;
  char *tmp, *val;

  x = (sys->x.val) ? sys->x : *cx;
  y = (sys->y.val) ? sys->y : *cy;

  val = mpui_sys_update (sys);
  if (!val)
    return;

  tmp = (char *) malloc (strlen (sys->caption) + strlen (val) + 4);
  sprintf (tmp, "%s : %s", sys->caption, val);
  string = mpui_string_new (NULL, tmp, MPUI_ENCODING_UTF8);
  free (tmp);

  str = mpui_str_new (string, x, y, 0, inf->info->font,
                      MPUI_FONT_SIZE_DEFAULT, NULL, NULL,
                      NULL, MPUI_DISPLAY_ALWAYS);

  mpui_container_elements_add ((mpui_container_t *) inf, 
                               (mpui_element_t *) str);

  *cx = x;
  (*cy).val = y.val + ((mpui_element_t *) str)->h.val;
}

static void
mpui_info_generate (mpui_t *mpui, mpui_inf_t *inf)
{
  mpui_tag_t **tag;
  mpui_pic_t **pic;
  stream_t* stream = NULL;
  demuxer_t *demuxer = NULL;
  mpui_coord_t x, y;
  int file_format;

  stream = open_stream (inf->filename, 0, &file_format);
  if (!stream)
    return;

  demuxer = demux_open (stream, file_format, -1, -1, -1, inf->filename);
  if (!demuxer)
    {
      free_stream (stream);
      return;
    }

  x = inf->info->x;
  y = inf->info->y;

  for (tag = inf->info->tags; *tag; tag++)
    mpui_info_update_tag (inf, *tag, &x, &y, demuxer);
  for (pic = inf->info->pics; *pic; pic++)
    mpui_info_update_pic (mpui, inf, *pic);

  free_demuxer (demuxer);
  free_stream (stream);
}

static void
mpui_info_generate_sys (mpui_t *mpui, mpui_inf_t *inf)
{
  mpui_sys_t **sys;
  mpui_coord_t x, y;

  x = inf->info->x;
  y = inf->info->y;

  for (sys = inf->info->sys; *sys; sys++)
    mpui_info_update_sys (inf, *sys, &x, &y);
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
mpui_info_filename (mpui_inf_t *inf, char *filename)
{
  if (filename)
    strncpy (inf->filename, filename, sizeof (inf->filename) - 1);
  else
    *inf->filename = '\0';

  inf->filename_id++;
  inf->next_timer = GetTimer () + 1000*inf->timer;
}

void
mpui_info_update (mpui_t *mpui, mpui_inf_t *inf)
{
  if (GetTimer() > inf->next_timer)
    {
      if (inf->last_filename_id != inf->filename_id)
        {
          mpui_info_clean (inf);
          if (*inf->filename)
            mpui_info_generate (mpui, inf);
          inf->last_filename_id = inf->filename_id;
        }
      else if (!mpui_list_empty (inf->info->sys))
        {
          mpui_info_clean (inf);
          mpui_info_generate_sys (mpui, inf);
          inf->next_timer = GetTimer () + 5000 * inf->timer;
        }
    }
}
