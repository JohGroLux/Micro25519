///////////////////////////////////////////////////////////////////////////////
// gfp_hlv_rvm.S: Halving (Div by 2) Modulo a 255-bit Pseudo-Mersenne Prime. //
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
// void gfp_hlv_rvm(uint32_t *r, const uint32_t *a);
//
// Description:
// ------------
// The function `gfp_hlv_rvm` computes the half $r = a/2 \bmod p$ of an element
// $a$ of a pseudo-Mersenne prime field. The prime $p$ is the 255-bit pseudo-
// Mersenne prime $p = 2^{255} - 19$. Operand $a$ is allowed to be larger than
// $p$. The result $r$ may not be fully reduced, but $r$ is always less than
// $2p$.
//
// Parameters:
// -----------
// `r`: pointer to array for the eight 32-bit words of the result $r$.
// `a`: pointer to array containing the eight 32-bit words of operand $a$.


///////////////////////////////////////////////////////////////////////////////
//////////////////////// REGISTER NAMES AND CONSTANTS /////////////////////////
///////////////////////////////////////////////////////////////////////////////


// Constant c of the pseudo-Mersenne prime: p = 2^k - c
.equ CCON, 19

// Register `rptr` holds the start address of array `r`
#define rptr a0
// Register `aptr` holds the start address of array `a`
#define aptr a1
// Register `mask` holds a mask for the addition of $p$
#define mask a2
// Registers `tmp0` and `tmp1` hold intermediate results
#define tmp0 a3
#define tmp1 a4
// Registers `dif0` and `dif1` hold two difference words
#define dif0 a5
#define dif1 a6
// Registers `bbit` holds the borrow-bit of subtraction
#define bbit a7


///////////////////////////////////////////////////////////////////////////////
/////////////////// MACROS FOR WORD-WISE HALVING OPERATIONS ///////////////////
///////////////////////////////////////////////////////////////////////////////


// The macro `HLVW_V1` loads the word `a[i]` from RAM and obtains a bit-mask,
// which is either 0 (if the LSB of `a[i]` is 0) or 0xFFFFFFFF (LSB of `a[i]`
// is 1). This bit-mask is put in register `mask`. Then, the constant $c$ is
// ANDed with `mask` and the result is subtracted from `a[i]`. The difference
// is put in `dif` and the borrow-bit in `bbo`.

.macro HLVW_V1 bbo:req, dif:req, i:req
    lw      \dif, \i(aptr)
    li      tmp0, CCON
    andi    tmp1, \dif, 1
    sub     mask, zero, tmp1
    and     tmp0, tmp0, mask
    sltu    \bbo, \dif, tmp0
    sub     \dif, \dif, tmp0
.endm


// The macro `HLVW_V2` loads the word `a[i]` from RAM and subtracts an incoming
// borrow-bit in `bbi` from it. The difference is put in `dif` and the outgoing
// borrow-bit in `bbo`. Then, `dif` is shifted 31 bits to the left and combined
// (i.e., XORed) with the 1-bit right-shifted previous difference in `old`. The
// result is stored in `r[i-1]` in RAM.

.macro HLVW_V2 bbo:req, dif:req, bbi:req, old:req, i:req
    lw      tmp0, \i(aptr)
    srli    \old, \old, 1
    sub     \dif, tmp0, \bbi
    sltu    \bbo, tmp0, \bbi
    slli    tmp1, \dif, 31
    or      \old, \old, tmp1
    sw      \old, (\i-4)(rptr)
.endm


// The macro `HLVW_V3` first subtracts the incoming borrow-bit in `bbi` from
// `mask` shifted 31 bits to the left. Since `mask` was either 0 or 0xFFFFFFFF
// before the shift, the shifted `mask` is either 0 or 0x80000000. However, the
// difference is always positive since a non-0 borrow-bit is only possible when
// `mask` was non-0. Then, the word `a[i]` is loaded from RAM and added to the
// difference. The double-length result is put in the `cry:sum` register-pair,
// whereby the upper word (in `cry`) is in the range [0, 1]. Thereafter, `sum`
// is shifted 31 bits to the left and and combined (i.e., XORed) with the 1-bit
// right-shifted previous difference in `old`. The result is stored in `r[i-1]`
// in RAM. Finally, `cry` is shifted 32 bits to the left and combined with the
// 1-bit right-shifted sum in `sum`. The result is stored in `r[i]` in RAM.

.macro HLVW_V3 cry:req, sum:req, bbi:req, old:req, i:req
    lw      \sum, \i(aptr)
    srli    \old, \old, 1
    slli    mask, mask, 31
    sub     mask, mask, \bbi
    add     \sum, \sum, mask
    sltu    \cry, \sum, mask
    slli    tmp0, \sum, 31
    xor     \old, \old, tmp0
    sw      \old, (\i-4)(rptr)
    srli    \sum, \sum, 1
    slli    \cry, \cry, 31
    xor     \sum, \sum, \cry
    sw      \sum, \i(rptr)
.endm


///////////////////////////////////////////////////////////////////////////////
///////////// SPEED-OPTIMIZED PRIME-FIELD HALVING (FULLY UNROLLED) ////////////
///////////////////////////////////////////////////////////////////////////////


// This implementation of (conditional) prime-field negation tightly integrates
// either an subtraction $p - a$ (LSB of `c` is 1) or an addition $p + a$ (LSB
// of `c` is is 0) with a fast reduction modulo $p = 2^{255} - 19$. It aims for
// high performance, which means the loop is fully unrolled. Each word of the
// array `a` is loaded from RAM exactly once, and each word of the result-array
// `r` is stored to RAM exactly once.

.text
.global gfp_hlv_rvm
.type gfp_hlv_rvm,%function
// .balign 8
gfp_hlv_rvm:
    HLVW_V1 bbit, dif0, 0
    HLVW_V2 bbit, dif1, bbit, dif0, 4
    HLVW_V2 bbit, dif0, bbit, dif1, 8
    HLVW_V2 bbit, dif1, bbit, dif0, 12
    HLVW_V2 bbit, dif0, bbit, dif1, 16
    HLVW_V2 bbit, dif1, bbit, dif0, 20
    HLVW_V2 bbit, dif0, bbit, dif1, 24
    HLVW_V3 bbit, dif1, bbit, dif0, 28
    ret


.end
