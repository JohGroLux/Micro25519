///////////////////////////////////////////////////////////////////////////////
// mpi_sub_rvm.S: Generic Subtraction of two Multi-Precision Integers.       //
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


// Function prototype:
// -------------------
// int mpi_sub_rvm(uint32_t *r, const uint32_t *a, const uint32_t *b, int len);
//
// Description:
// ------------
// The function `mpi_sub_rvm` computes the difference $r = a - b$ of two multi-
// precision integers $a$ and $b$. Operands $a$ and $b$, as well as the result
// `r`, have a length of `len` words. Negative results are represented in two's
// complement form and have a return value of 1.
//
// Parameters:
// -----------
// `r`: pointer to array for the 32-bit words of the result $r$.
// `a`: pointer to array containing the 32-bit words of operand $a$.
// `b`: pointer to array containing the 32-bit words of operand $b$.
// `len`: number of 32-bit words of `a`, `b`, and `r` (must be >= 1).
//
// Return value:
// -------------
// Borrow-bit: 1 when $a < b$ (i.e., $r$ is negative), 0 otherwise.


///////////////////////////////////////////////////////////////////////////////
//////////////////////// REGISTER NAMES AND CONSTANTS /////////////////////////
///////////////////////////////////////////////////////////////////////////////


// Register `rptr` holds the start address of array `r`
#define rptr a0
// Register `aptr` holds the start address of array `a`
#define aptr a1
// Register `bptr` holds the start address of array `b`
#define bptr a2
// Register `len` holds the word-length of the operands
#define len a3
// Registers `tmp1` and `tmp2` hold intermediate results
#define tmp1 a4
#define tmp2 a5
// Registers `dif` and `bor` hold difference and borrow
#define dif a6
#define bor a7


///////////////////////////////////////////////////////////////////////////////
//////////// FLEXIBLE MULTI-PRECISION SUBTRACTION (SIZE-OPTIMIZED) ////////////
///////////////////////////////////////////////////////////////////////////////


// This implementation of multi-precision integer subtraction $r = a - b$ aims
// for high flexibility, which means it supports operands of arbitrary length,
// specified by parameter `len`. Each word of the arrays `a` and `b` is loaded
// from RAM exactly once, and each word of the result-array `r` is stored to
// RAM exactly once.


.text
.global mpi_sub_rvm
.type mpi_sub_rvm,%function
// .balign 8
mpi_sub_rvm:
    mv      bor, zero
.LLOOP:
    lw      dif, 0(aptr)
    lw      tmp1, 0(bptr)
    addi    aptr, aptr, 4
    addi    bptr, bptr, 4
    sltu    tmp2, dif, tmp1
    sub     dif, dif, tmp1
    sltu    tmp1, dif, bor
    sub     dif, dif, bor
    add     bor, tmp1, tmp2
    addi    len, len, -1
    sw      dif, 0(rptr)
    addi    rptr, rptr, 4
    bnez    len, .LLOOP
    mv      a0, bor
    ret


.end
