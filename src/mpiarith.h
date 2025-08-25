///////////////////////////////////////////////////////////////////////////////
// mpiarith.h: Basic functions for Multi-Precision Integer (MPI) arithmetic. //
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

#ifndef _MPIARITH_H
#define _MPIARITH_H

#include "config.h"

// utility functions (only in C)
int  mpi_from_hex(Word *r, const char *hexstr, int len);
void mpi_to_hex(char *hexstr, const Word *a, int len);
void mpi_print(const char *prefix, const Word *a, int len);
void mpi_copy(Word *r, const Word *a, int len);
void mpi_setw(Word *r, Word a, int len);
int  mpi_cmpw(const Word *a, Word b, int len);
int  mpi_cmp(const Word *a, const Word *b, int len);

// arithmetic functions with C implementations only
int  mpi_add(Word *r, const Word *a, const Word *b, int len);
int  mpi_cadd(Word *r, const Word *a, const Word *b, int add, int len);
void mpi_mul(Word *r, const Word *a, const Word *b, int len);

// arithmetic functions with C and ASM implementations
#if defined(M25519_ASSEMBLY)  // ASM functions are available
extern int mpi_shr_asm(Word *r, const Word *a, int len);
#define mpi_shr(r, a, len) mpi_shr_asm((r), (a), (len))
extern int mpi_sub_asm(Word *r, const Word *a, const Word *b, int len);
#define mpi_sub(r, a, b, len) mpi_sub_asm((r), (a), (b), (len))
#else  // ASM functions are not available or not used
int mpi_shr(Word *r, const Word *a, int len);
int mpi_sub(Word *r, const Word *a, const Word *b, int len);
#endif

#endif
