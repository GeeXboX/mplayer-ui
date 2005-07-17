/*
 *  mpui_slideshow.h: libmpui slideshow management.
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

#ifndef MPUI_SLIDESHOW_H
#define MPUI_SLIDESHOW_H

#include "mpui_struct.h"

void mpui_slideshow_path (mpui_slideshow_t *slideshow, char *path);
void mpui_slideshow_pause (mpui_slideshow_t *slideshow);
void mpui_slideshow_prev (mpui_slideshow_t *slideshow);
void mpui_slideshow_next (mpui_slideshow_t *slideshow);
void mpui_slideshow_mode (mpui_slideshow_t *slideshow, char *mode);
void mpui_slideshow_rotate (mpui_slideshow_t *slideshow, int rotate);
void mpui_slideshow_update (mpui_t *mpui, mpui_slideshow_t *slideshow,
                            mpui_size_t context_x, mpui_size_t context_y);
void mpui_slideshow_cleanup (mpui_slideshow_t *slideshow);

#endif  /* MPUI_SLIDESHOW_H */
