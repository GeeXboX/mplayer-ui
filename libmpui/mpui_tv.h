/*
 *  mpui_tv.h: libmpui tv and dvb properties.
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

#ifndef MPUI_TV_H
#define MPUI_TV_H

void mpui_tv_analog_channels_generate (mpui_menu_t *menu,
                                       mpui_size_t *mx, mpui_size_t *my,
                                       mpui_size_t *mw, mpui_size_t *mh,
                                       mpui_coord_t *spacing,
                                       mpui_string_t *empty);

void mpui_tv_dvb_channels_generate (mpui_menu_t *menu,
                                    mpui_size_t *mx, mpui_size_t *my,
                                    mpui_size_t *mw, mpui_size_t *mh,
                                    mpui_coord_t *spacing,
                                    mpui_string_t *empty);

#endif  /* MPUI_TV_H */
