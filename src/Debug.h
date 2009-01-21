/*
 * Copyright 2008, 2009, Dominik Geyer
 *
 * This file is part of HoldingNuts.
 *
 * HoldingNuts is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HoldingNuts is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HoldingNuts.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Dominik Geyer <dominik.geyer@holdingnuts.net>
 */


#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>

#ifndef _MSC_VER

# define dbg_print(level, s, args...) \
	do { fprintf(stderr, "[%s]: "s"\n", level, ##args) ; fflush(stderr); } while(0)
#else /* _MSC_VER */
# define dbg_print(level, s, ...) \
	do { fprintf(stderr, "[%s]: "s"\n", level, __VA_ARGS__) ; fflush(stderr); } while(0)
#endif /* _MSC_VER */

#endif /* _DEBUG_H */

