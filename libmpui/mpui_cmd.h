/*
 *  mpui_cmd.h: libmpui actions execution.
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

#ifndef MPUI_CMD_H
#define MPUI_CMD_H

#include "mpui_struct.h"

void mpui_cmd_screen (mpui_t *mpui, char *screen_id);
void mpui_cmd_popup (mpui_t *mpui, char *popup_id);
void mpui_cmd_popup_close (mpui_t *mpui);
void mpui_cmd_hide (mpui_t *mpui, char *element_id);
void mpui_cmd_show (mpui_t *mpui, char *element_id);
void mpui_cmd_hide_switch (mpui_t *mpui, char *element_id);
void mpui_cmd_info (mpui_t *mpui, char *filename);

#endif  /* MPUI_CMD_H */
