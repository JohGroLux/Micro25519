///////////////////////////////////////////////////////////////////////////////
// config.h: Configuration settings and basic datatypes (e.g., Word, Point). //
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


#ifndef _CONFIG_H
#define _CONFIG_H


#include <stdint.h>


// Micro25519 uses the pseudo-Mersenne prime $p = 2^k - c$ with $k = 255$ and
// $c = 19$. This prime has a length of 255 bits, which means an element of the
// prime field GF(p) can be stored in a Word-array consisting of eight words of
// WSIZE = 32 bits (see below).

#define CONSTK 255
#define CONSTC 19


// Micro25519 will use optimized Assembly implementations of the performance-
// critical MPI and field arithmetic if `M25519_USE_ASM` is defined, provided
// that the target architecture is either AVR8, MSP430, ARMv7-M, or RV32IM. If
// this is not the case, or when `M25519_USE_ASM` is not defined, the ordinary
// C implementation of the MPI and field arithmetic will be used.

// #define M25519_USE_ASM


// Micro25519 will use Variable-Length Arrays (VLAs) for temporary/intermediate
// operands if `M25519_USE_VLA` is defined and the length of these operands is
// not known or not fixed at compile time. If `M25519_USE_VLA` is not defined,
// static arrays of a certain maximum length will be used.

// #define M25519_USE_VLA


// When Micro25519 is compiled for one of the four target architectures that
// are supported with optimized Assembly code (AVR8, MSP430, ARMv7-M, RV32IM)
// and `M25519_USE_ASM` is defined, then the Assembly implementation of the
// performance-critical field/integer arithmetic is used.

#if defined(M25519_USE_ASM)
#if (defined(__AVR) || defined(__AVR__))
#define M25519_TARGET AVR8BIT
#define M25519_ASSEMBLY
#elif (defined(__MSP430__) || defined(__ICC430__))
#define M25519_TARGET MSP430
#define M25519_ASSEMBLY
#elif (defined(__arm__) || defined(_M_ARM))
#define M25519_TARGET ARMV7M
#define M25519_ASSEMBLY
#elif (defined(__riscv) && (__riscv_xlen == 32))
#define M25519_TARGET RV32IM
#define M25519_ASSEMBLY
#endif // #if (defined(__AVR) || ...
#endif // #if defined(M25519_USE_ASM)


//#ifndef NDEBUG
#define M25519_DBG_PRINT
//#endif


// Error codes

#define M25519_NO_ERROR   0
#define M25519_ERR_HEXSTR 1
#define M25519_ERR_INVERS 2
#define M25519_ERR_MPOINT 4
#define M25519_ERR_TPOINT 8
#define M25519_ERR_SCALAR 16
#define M25519_ERR_TVFILE 32


// `Word` is the basic data type used to represent a multiple-precision integer
// or an element of a prime field.

typedef uint32_t Word;     // a single-length word is 32 bits long
typedef uint64_t DWord;    // a double-length word is 64 bits long
typedef int64_t SDWord;    // a signed double-length word is 64 bits long
#define WSIZE 32           // the word-size is 32 bits


// An element of the prime field GF(p) with $p = 2^k - c$ is stored in a Word-
// array consisting of `LEN` Words.

#define LEN ((CONSTK + WSIZE - 1)/WSIZE)


// `Point` is a simple but highly flexible C structure to represent a point on
// an elliptic curve in affine, projective, or extended projective coordinates.
// Other ECC libraries use different data structures for affine and projective
// points, commonly a `struct` with two or three fixed-length `Word` arrays for
// the coordinates, but a `struct` composed of an integer specifying the number
// of coordinates (i.e., the dimension of the point) and a pointer to a single
// `Word` array containing all coordinates offers more flexibility when curves
// and fields of varying order have to be supported. Typical use cases for the
// different dimensions are as follows:
// dim = 1: x-coordinate only (e.g., X25519 key exchange)
// dim = 2: conventional affine (x,y) or projective [X:Z] coordinate system
// dim = 3: projective [X:Y:Z] or extended affine (u,v,w) coordinate system
// dim = 4: same as dim = 3, but with further space for an intermediate result
// dim = 5: extended projective [X:Y:Z:E:H] coordinate system (where E*H = T)
// dim = 6: same as dim = 5, but with further space for an intermediate result

typedef struct point {
  int dim;    // dimension: 1 <= dim <= 6
  Word *xyz;  // pointer to coordinates-array
} Point;


#endif  // _CONFIG_H
