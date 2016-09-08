/* Copyright (C) 2006-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Minimum possible text address in AIX.  */
#define AIX_TEXT_SEGMENT_BASE 0x10000000

/* For sighandler */

/* STKMIN(minimum stack size) is 56 for 32-bit process
   and iar offset under sc_jmpbuf.jmp_context is 40 under the sigconext structure.
   ie offsetof(struct sigcontext, sc_jmpbuf.jmp_context.iar).
   so PC offset in this case is STKIM+iar offset, which is 96 */
#define SIG_FRAME_PC_OFFSET 96
#define SIG_FRAME_LR_OFFSET 108
/* STKMIN+grp1 offset, which is 56+228=284 */
#define SIG_FRAME_FP_OFFSET 284

/* 64 bit process */
/* STKMIN64  is 112 and iar offset is 312. So 112+312=424 */
#define SIG_FRAME_PC_OFFSET64 424
/* STKMIN64+grp1 offset. 112+56=168 */
#define SIG_FRAME_FP_OFFSET64 168 

#define EXT_CONTEXT64 452
