/*
 *  mpui_render.h: libmpui screen renderer.
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

#ifndef MPUI_RENDER_H
#define MPUI_RENDER_H

#include <stdlib.h>
#include <string.h>
#include "../libmpcodecs/mp_image.h"
#include "mpui_struct.h"

int mpui_render_screen (mpui_t *mpui, mp_image_t *mpi);

#endif /* MPUI_RENDER_H */
