/*
 *  mpui_parser.h: libmpui theme file parser.
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

#ifndef MPUI_PARSER_H
#define MPUI_PARSER_H

#include "mpui_struct.h"
#include "config.h"

#define MPUI_DATADIR MPLAYER_DATADIR"/mpui/"

mpui_t *mpui_parse_config (mpui_t *ui, char *buffer, int width, int height, int format);
mpui_t *mpui_parse_config_file (char *filename, int width, int height, int format);

void mpui_recompute_coord (mpui_t *mpui, mpui_element_t *element,
                           mpui_size_t w, mpui_size_t h, int dynamic,
                           int focus, int really_focus);
void mpui_clip (mpui_t *mpui, mpui_element_t *element,
                mpui_size_t x, mpui_size_t y, int dynamic);

#endif /* MPUI_PARSER_H */
