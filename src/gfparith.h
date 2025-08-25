///////////////////////////////////////////////////////////////////////////////
// gfparith.h: Basic arithmetic operations in a 255-bit prime field GF(p).   //
// This file is part of Micro25519, a lightweight implementation of X25519   //
// key exchange and Ed25519 signatures for 8/16/32-bit microcontrollers.     //
// Version 1.0.0 (13-06-25), see <http://github.com/johgrolux/> for updates. //
// License: GPLv3 (see LICENSE file), other licenses available upon request. //
// Author: Johann Groszschaedl (in personal capacity).                       //
// ------------------------------------------------------------------------- //
// This program is free software: you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation, either version 3 of the License, or (at your    //
// option) any later version. This program is distributed in the hope that   //
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied     //
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the  //
// GNU General Public License for more details. You should have received a   //
// copy of the GNU General Public License along with this program. If not,   //
// see <http://www.gnu.org/licenses/>.                                       //
///////////////////////////////////////////////////////////////////////////////

#ifndef _GFPARITH_H
#define _GFPARITH_H

#include "config.h"

// prototypes of functions with C implementations only
void gfp_setp(Word *r);
int  gfp_cmpp(const Word *a);
void gfp_fred(Word *r, const Word *a);
int  gfp_cmp(const Word *a, const Word *b);
int  gfp_inv(Word *r, const Word *a);

// prototypes of functions with C and ASM implementations
#if defined(M25519_ASSEMBLY)  // ASM functions are available
extern void gfp_add_asm(Word *r, const Word *a, const Word *b);
#define gfp_add(r, a, b) gfp_add_asm((r), (a), (b))
extern void gfp_cneg_asm(Word *r, const Word *a, int neg);
#define gfp_cneg(r, a, neg) gfp_cneg_asm((r), (a), (neg))
extern void gfp_hlv_asm(Word *r, const Word *a);
#define gfp_hlv(r, a) gfp_hlv_asm((r), (a))
extern void gfp_mul_asm(Word *r, const Word *a, const Word *b);
#define gfp_mul(r, a, b) gfp_mul_asm((r), (a), (b))
extern void gfp_mul32_asm(Word *r, const Word *a, const Word *b);
#define gfp_mul32(r, a, b) gfp_mul32_asm((r), (a), (b))
extern void gfp_sqr_asm(Word *r, const Word *a);
#define gfp_sqr(r, a) gfp_sqr_asm((r), (a))
extern void gfp_sub_asm(Word *r, const Word *a, const Word *b);
#define gfp_sub(r, a, b) gfp_sub_asm((r), (a), (b))
#else  // ASM functions are not available or not used
void gfp_add(Word *r, const Word *a, const Word *b);
void gfp_cneg(Word *r, const Word *a, int neg);
void gfp_hlv(Word *r, const Word *a);
void gfp_mul(Word *r, const Word *a, const Word *b);
void gfp_mul32(Word *r, const Word *a, const Word *b);
void gfp_sqr(Word *r, const Word *a);
void gfp_sub(Word *r, const Word *a, const Word *b);
#endif

#endif
