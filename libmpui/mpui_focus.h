/*
 *  mpui_focus.h: libmpui focus manager.
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

#ifndef MPUI_FOCUS_H
#define MPUI_FOCUS_H

#include "mpui_struct.h"


int mpui_focus_box_first (mpui_screen_t *screen);
void mpui_focus_box_next (mpui_screen_t *screen);
void mpui_focus_box_previous (mpui_screen_t *screen);

int mpui_focus_first (mpui_focus_box_t *focus_box);
void mpui_focus_next (mpui_focus_box_t *focus_box);
void mpui_focus_previous (mpui_focus_box_t *focus_box);
void mpui_focus_next_line (mpui_focus_box_t *focus_box);
void mpui_focus_previous_line (mpui_focus_box_t *focus_box);
void mpui_focus_element (mpui_focus_box_t *focus_box, mpui_element_t *element);

void mpui_focus_action_exec (mpui_focus_box_t *focus_box,
                             mpui_action_when_t when);

int mpui_is_focused (mpui_screen_t *screen, mpui_element_t *element);
int mpui_is_really_focused (mpui_screen_t *screen, mpui_element_t *element);

#endif  /* MPUI_FOCUS_H */
