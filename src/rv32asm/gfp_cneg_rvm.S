///////////////////////////////////////////////////////////////////////////////
// gfp_cneg_rvm.S: Conditional Negation Mod a 255-bit Pseudo-Mersenne Prime. //
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
// void gfp_cneg_asm(uint32_t *r, const uint32_t *a, int c);
//
// Description:
// ------------
// The function `gfp_cneg_asm` computes the negative $r = a - b \bmod p$ of an
// element $a$ of a pseudo-Mersenne prime field if the LSB of parameter `c` is
// 1. On the other hand, if the LSB of `c` is 0, the result is $a$. The prime
// $p$ is the 255-bit pseudo-Mersenne prime $p = 2^{255} - 19$. Operand $a$ is
// allowed to be larger than $p$. The result $r$ may not be fully reduced, but
// $r$ is always less than $2p$.
//
// Parameters:
// -----------
// `r`: pointer to array for the eight 32-bit words of the result $r$.
// `a`: pointer to array containing the eight 32-bit words of operand $a$.
// `c`: condition bit (only the LSB is considered).


///////////////////////////////////////////////////////////////////////////////
//////////////////////// REGISTER NAMES AND CONSTANTS /////////////////////////
///////////////////////////////////////////////////////////////////////////////


// Constant c of the pseudo-Mersenne prime: p = 2^k - c
.equ CCON, 19

// Register `rptr` holds the start address of array `r`
#define rptr a0
// Register `aptr` holds the start address of array `a`
#define aptr a1
// Register `mask` holds a mask for conditional negation
#define mask a2
// Registers `tmp0` and `tmp1` hold intermediate results
#define tmp0 a3
#define tmp1 a4
// Registers `sumw` and `cryw` hold sum and carry words
#define sumw a5
#define cryw a6
// Register `rmsw` holds the highest word of the result
#define rmsw a7


///////////////////////////////////////////////////////////////////////////////
///////////// MACROS FOR WORD-WISE CONDITIONAL NEGATION OPERATIONS ////////////
///////////////////////////////////////////////////////////////////////////////


// The macro `NEGW_V1` loads the word `a[i]` from RAM, XORs it with `mask` (0
// or 0xFFFFFFFF), and adds the XORed word to an incoming carry in `cin` (which
// is non-negative). Then, the constants $2c$ and $2c - 1$ are subtracted from
// the sum (the latter is ANDed with `mask` before the subtraction is carried
// out). The double-length result is put in the `shi:slo` register-pair. This
// result can be negative, whereby the upper word (in `shi`) is always in the
// range $[-1, 1]$. The lower word of the result is stored in `r[i]` in RAM.
// NOTE: Register `slo` has to be different from `cin`.

.macro NEGW_V1 shi:req, slo:req, ccw:req, cin:req, i:req
    slli    \ccw, \ccw, 1
    addi    tmp0, \ccw, -1
    and     tmp0, tmp0, mask
    add     tmp1, \ccw, tmp0
    lw      \slo, \i(aptr)
    sltu    tmp0, \cin, tmp1
    sub     tmp1, \cin, tmp1
    xor     \slo, \slo, mask
    add     \slo, \slo, tmp1
    sltu    \shi, \slo, tmp1
    sub     \shi, \shi, tmp0
    sw      \slo, \i(rptr)
.endm


// The macro `NEGW_V2` loads the word `a[i]` from RAM, XORs it with `mask` (0
// or 0xFFFFFFFF), and adds the XORed word to an incoming carry in `cin` (which
// is signed and can therefore be negative). The double-length result is put in
// the `shi:slo` register-pair. This result can be negative, whereby the upper
// word (in `shi`) is always in the range $[-2, 1]$. The lower word of the
// result is stored in `r[i]` in RAM.

.macro NEGW_V2 shi:req, slo:req, cin:req, i:req
    lw      \slo, \i(aptr)
    srai    tmp0, \cin, 1
    xor     \slo, \slo, mask
    add     \slo, \slo, \cin
    sltu    \shi, \slo, \cin
    add     \shi, \shi, tmp0
    sw      \slo, \i(rptr)
.endm


// The macro `NEGW_V3` first obtains a bit-mask, which is either 0 (if the LSB
// of parameter `c` is 0) or 0xFFFFFFFF (LSB of parameter `c` is 1), and puts
// it in register `mask`. Then, the word `a[i]` is loaded from RAM, XORed with
// `mask`, and the XORed word is added to 0xFFFFFFFC. The double-length result
// is put in the `shi:slo` register-pair. This result is always positive,
// whereby the upper word (in `shi`) is in the range [0, 1].

.macro NEGW_V3 shi:req, slo:req, i:req
    lw      tmp0, \i(aptr)
    andi    mask, mask, 1
    sub     mask, zero, mask
    xor     tmp0, tmp0, mask
    addi    \slo, tmp0, -4
    sltu    \shi, \slo, tmp0
.endm


// The macro `NEGW_V4` adds the word `cin` along with 4 to `sum` and stores the
// result in `r[i]` in RAM. 
// NOTE: the bit-length of `sum` and `cin` can be up to 31.

.macro NEGW_V4 sum:req, cin:req, i:req
    add     \sum, \sum, \cin
    addi    \sum, \sum, 4
    sw      \sum, \i(rptr)
.endm


// The macro `MULHIXC` first splits the double-length word in `dhi:dlo` into a
// 31-bit lower part and an upper part. The lower part is put in `rlo`, while
// the upper part is multiplied by the constant $c = 19$ and the single-word
// product is put in `rhi`.
// NOTE: the bit-length of `dhi` can be up to $32 - \log_2(c) - 1$, e.g., for
// $c = 19$, `dhi` can be up to 26 bits long.

.macro MULHIXC rhi:req, rlo:req, ccw:req, dhi:req, dlo:req
    srli    tmp0, \dlo, 31
    slli    \rhi, \dhi, 1
    add     \rhi, \rhi, tmp0
    slli    \rlo, \dlo, 1
    mul     \rhi, \rhi, \ccw
    srli    \rlo, \rlo, 1
 .endm


///////////////////////////////////////////////////////////////////////////////
////// SPEED-OPTIMIZED PRIME-FIELD CONDITIONAL NEGATION (FULLY UNROLLED) //////
///////////////////////////////////////////////////////////////////////////////


// This implementation of (conditional) prime-field negation tightly integrates
// either an subtraction $p - a$ (LSB of `c` is 1) or an addition $p + a$ (LSB
// of `c` is is 0) with a fast reduction modulo $p = 2^{255} - 19$. It aims for
// high performance, which means the loop is fully unrolled. Each word of the
// array `a` is loaded from RAM exactly once, and each word of the result-array
// `r` is stored to RAM exactly once.

.text
.global gfp_cneg_asm
.type gfp_cneg_asm,%function
// .balign 8
gfp_cneg_asm:
    NEGW_V3 cryw, sumw, 28
    li      tmp1, CCON  // tmp1 = 19
    MULHIXC cryw, rmsw, tmp1, cryw, sumw
    NEGW_V1 cryw, sumw, tmp1, cryw, 0
    NEGW_V2 cryw, sumw, cryw, 4
    NEGW_V2 cryw, sumw, cryw, 8
    NEGW_V2 cryw, sumw, cryw, 12
    NEGW_V2 cryw, sumw, cryw, 16
    NEGW_V2 cryw, sumw, cryw, 20
    NEGW_V2 cryw, sumw, cryw, 24
    NEGW_V4 rmsw, cryw, 28
    ret


.end
