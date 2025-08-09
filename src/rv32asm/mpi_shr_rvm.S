///////////////////////////////////////////////////////////////////////////////
// mpi_shr_rvm.S: Generic 1-bit Right-Shift of a Multi-Precision Integer.    //
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
// int mpi_shr_asm(uint32_t *r, const uint32_t *a, int len);
//
// Description:
// ------------
// The function `mpi_shr_asm` computes the 1-bit right-shift $r = a >> 1$ of a
// multi-precision integer $a$. Operands $a$, as well as the result `r`, have a
// length of `len` words. The right-shift is a logical shift operations, which
// means the MSB of result-word `r[len-1]` is always 0.
//
// Parameters:
// -----------
// `r`: pointer to array for the eight 32-bit words of the result $r$.
// `a`: pointer to array containing the eight 32-bit words of operand $a$.
// `len`: number of 32-bit words of `a` and `r` (must be >= 1).
//
// Return value:
// -------------
// The bit shifted out from word `a[0]` of operand `a` (either 0 or 1).


///////////////////////////////////////////////////////////////////////////////
//////////////////////// REGISTER NAMES AND CONSTANTS /////////////////////////
///////////////////////////////////////////////////////////////////////////////


// Register `rptr` holds the start address of array `r`
#define rptr a0
// Register `aptr` holds the start address of array `a`
#define aptr a1
// Register `len` holds the word-length of the operands
#define len a2
// Register `tmp` holds an intermediate/temporary result
#define tmp a3
// Registers `dif` and `bor` hold difference and borrow
#define cur a4
#define old a5
// Register `rval` holds the return value (LSB of a[0])
#define rval a6


///////////////////////////////////////////////////////////////////////////////
///////// FLEXIBLE MULTI-PRECISION 1-BIT RIGHT-SHIFT (SIZE-OPTIMIZED) /////////
///////////////////////////////////////////////////////////////////////////////


// This implementation of multi-precision 1-bit right-shift $r = a >> 1$ aims
// for high flexibility, which means it supports operands of arbitrary length,
// specified by parameter `len`. Each word of the array `a` is loaded from RAM
// exactly once, and each word of the result-array `r` is stored to RAM exactly
// once.


.text
.global mpi_shr_asm
.type mpi_shr_asm,%function
// .balign 8
mpi_shr_asm:
    lw      old, 0(aptr)
    addi    aptr, aptr, 4
    addi    len, len, -1
    andi    rval, old, 1
    srli    old, old, 1
    beqz    len, .LTERM
.LLOOP:
    lw      cur, 0(aptr)
    addi    aptr, aptr, 4
    addi    len, len, -1
    slli    tmp, cur, 31
    or      tmp, tmp, old
    srli    old, cur, 1
    sw      tmp, 0(rptr)
    addi    rptr, rptr, 4
    bnez    len, .LLOOP
.LTERM:
    sw      old, 0(rptr)
    mv      a0, rval
    ret


.end
