/*
 *  mpui_browser.h: libmpui file browser management.
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

#ifndef MPUI_BROWSER_H
#define MPUI_BROWSER_H

#include "mpui_struct.h"

void mpui_browser_generate (mpui_t *mpui, mpui_browser_t *browser);
void mpui_browser_clean (mpui_browser_t *browser);
void mpui_browser_update (mpui_t *mpui, mpui_mnu_t *mnu);
void mpui_browser_cd (mpui_t *mpui, char *directory);

#endif  /* MPUI_BROWSER_H */
