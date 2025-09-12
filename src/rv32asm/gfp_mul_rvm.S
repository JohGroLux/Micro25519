///////////////////////////////////////////////////////////////////////////////
// gfp_mul_rvm.S: Multiplication Modulo a 255-bit Pseudo-Mersenne Prime.     //
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
// void gfp_mul_asm(uint32_t *r, const uint32_t *a, const uint32_t *b);
//
// Description:
// ------------
// The function `gfp_mul_asm` computes the product $r = a \cdot b \bmod p$ of
// two elements $a$ and $b$ of a pseudo-Mersenne prime field. The prime $p$ is
// the 255-bit pseudo-Mersenne prime $p = 2^{255} - 19$. Operands $a$ and $b$
// are allowed to be larger than $p$. The result $r$ may not be fully reduced,
// but $r$ is always less than $2p$.
//
// Parameters:
// -----------
// `r`: pointer to array for the eight 32-bit words of the result $r$.
// `a`: pointer to array containing the eight 32-bit words of operand $a$.
// `b`: pointer to array containing the eight 32-bit words of operand $b$.


///////////////////////////////////////////////////////////////////////////////
//////////////////////// REGISTER NAMES AND CONSTANTS /////////////////////////
///////////////////////////////////////////////////////////////////////////////


// Constant c of the pseudo-Mersenne prime: p = 2^k - c
.equ CONSTC, 19

// Register `rptr` holds the start address of array `r`
#define rptr a0
// Register `aptr` holds the start address of array `a`
#define aptr a1
// Register `bptr` holds the start address of array `b`
#define bptr a2
// Registers `tmp0` and `tmp1` hold temporary variables
#define tmp0 s0
#define tmp1 s1
// Register `rcon` holds the constant c of the PM-prime
#define rcon s2
// Registers `aw00` to `aw07` hold words of operand `a`
#define aw00 s2
#define aw01 s3
#define aw02 s4
#define aw03 s5
#define aw04 s6
#define aw05 s7
#define aw06 s8
#define aw07 s9
// Register `bw0j` holds one single word of operand `b`
#define bw0j s1
// Registers `rw00` to `rw15` hold words of the product
#define rw00 s10
#define rw01 s11
#define rw02 a3
#define rw03 a4
#define rw04 a5
#define rw05 a6
#define rw06 a7
#define rw07 t0
#define rw08 t1
#define rw09 t2
#define rw10 t3
#define rw11 t4
#define rw12 t5
#define rw13 t6
#define rw14 a1
#define rw15 a2


///////////////////////////////////////////////////////////////////////////////
///////////////// MACROS FOR WORD-WISE MULTIPLY-ADD OPERATIONS ////////////////
///////////////////////////////////////////////////////////////////////////////


// The macro `MADD_V1` multiplies the word `aiw` by `bjw` and puts the product
// in the `rhi:rlo` register-pair (i.e., this macro performs a multiply-add
// operation where 0 is added to the product, i.e., a normal multiplication).
// NOTE: Registers `rhi` and `rlo` have to be different from `aiw` and `bjw`.

.macro MADD_V1 rhi:req, rlo:req, aiw:req, bjw:req
    mul     \rlo, \aiw, \bjw
    mulhu   \rhi, \aiw, \bjw
.endm


// The macro `MADD_V2` multiplies the word `aiw` by `bjw` and adds the word
// `c0w` to the product. The double-length result is put in the `rhi:rlo`
// register-pair.
// NOTE: Registers `rhi` and `rlo` have to be different from `aiw` and `bjw`.
// NOTE: Register `c0w` is (generally) the same as register `rlo`.

.macro MADD_V2 rhi:req, rlo:req, aiw:req, bjw:req, c0w:req
    mul     tmp0, \aiw, \bjw
    mulhu   \rhi, \aiw, \bjw
    add     \rlo, \c0w, tmp0
    sltu    tmp0, \rlo, tmp0
    add     \rhi, \rhi, tmp0
.endm


// The macro `MADD_V3` multiplies the word `aiw` by `bjw` and adds the two
// words `c0w` and `d0w` to the product. The double-length result is put in the
// `rhi:rlo` register-pair.
// NOTE: Registers `rhi` and `rlo` have to be different from `aiw` and `bjw`.
// NOTE: Register `c0w` is (generally) the same as register `rlo`, and register
// `d0w` is (generally) the same as register `rhi`.

.macro MADD_V3 rhi:req, rlo:req, aiw:req, bjw:req, d0w:req, c0w:req
    add     tmp0, \d0w, \c0w
    mulhu   \rhi, \aiw, \bjw
    sltu    \rlo, tmp0, \c0w
    add     \rhi, \rhi, \rlo
    mul     \rlo, \aiw, \bjw
    add     \rlo, \rlo, tmp0
    sltu    tmp0, \rlo, tmp0
    add     \rhi, \rhi, tmp0
.endm


///////////////////////////////////////////////////////////////////////////////
//////////// MACROS FOR LOW-LEVEL OPERATIONS FOR MODULAR REDUCTION ////////////
///////////////////////////////////////////////////////////////////////////////


// The macro `MULHIXC` first splits the double-length word in `dhi:dlo` into a
// 31-bit lower part and an upper part. The lower part is put in `rlo`, while
// the upper part is multiplied by the constant $c$ (in register `rcon`) and
// the single-word product is put in `rhi`.
// NOTE: The bit-length of `dhi` can be up to $32 - \log_2(c) - 1$, e.g., for
// $c = 19$, `dhi` can be up to 26 bits long.
// NOTE: Register `rcon` contains $2c$ instead of $c$ and must, therefore, be
// halved before the multiplication.

.macro MULHIXC rhi:req, rlo:req, dhi:req, dlo:req
    srli    tmp0, \dlo, 31
    slli    \rhi, \dhi, 1
    add     \rhi, \rhi, tmp0
    srli    tmp0, rcon, 1
    slli    \rlo, \dlo, 1
    mul     \rhi, \rhi, tmp0
    srli    \rlo, \rlo, 1
.endm


///////////////////////////////////////////////////////////////////////////////
///////////////// MACROS FOR ROW-WISE MULTIPLY-ADD OPERATIONS /////////////////
///////////////////////////////////////////////////////////////////////////////


// The macro `MROW_V1` multiplies the 8-word operand `aw00`-`aw07` by a single
// word (held in `bw0j`) and puts the 9-word product in `p0w`-`p8w`. This macro
// corresponds to the operation $r = a \cdot b_0$ performed by the very first
// iteration of the outer loop of the operand-scanning method. The word $b_0$
// of the multiplier is loaded to `bjw` via base-address `bptr` and offset `j`.

.macro MROW_V1 p8w:req, p7w:req, p6w:req, p5w:req, p4w:req, p3w:req, p2w:req, \
               p1w:req, p0w:req, j:req
    lw      bw0j, \j(bptr)
    MADD_V1 \p1w, \p0w, aw00, bw0j
    MADD_V2 \p2w, \p1w, aw01, bw0j, \p1w
    MADD_V2 \p3w, \p2w, aw02, bw0j, \p2w
    MADD_V2 \p4w, \p3w, aw03, bw0j, \p3w
    MADD_V2 \p5w, \p4w, aw04, bw0j, \p4w
    MADD_V2 \p6w, \p5w, aw05, bw0j, \p5w
    MADD_V2 \p7w, \p6w, aw06, bw0j, \p6w
    MADD_V2 \p8w, \p7w, aw07, bw0j, \p7w
.endm


// The macro `MROW_V2` multiplies the 8-word operand `aw00`-`aw07` by a single
// word (held in `bw0j`) and adds the 9-word product to operand `p0w`-`p8w`.
// This macro corresponds to the operation $r = r + a \cdot b_j \cdot 2^{32j}$
// for $1 \leq j < 8$ performed by the seven last iterations of the outer loop
// of the operand-scanning method. The word $b_j$ of the multiplier is loaded
// to `bjw` via base address `bptr` and offset `j`. The word `p8w` is used for
// temporary results.

.macro MROW_V2 p8w:req, p7w:req, p6w:req, p5w:req, p4w:req, p3w:req, p2w:req, \
               p1w:req, p0w:req, j:req
    lw      bw0j, \j(bptr)
    MADD_V2 \p8w, \p0w, aw00, bw0j, \p0w
    MADD_V3 \p8w, \p1w, aw01, bw0j, \p8w, \p1w
    MADD_V3 \p8w, \p2w, aw02, bw0j, \p8w, \p2w
    MADD_V3 \p8w, \p3w, aw03, bw0j, \p8w, \p3w
    MADD_V3 \p8w, \p4w, aw04, bw0j, \p8w, \p4w
    MADD_V3 \p8w, \p5w, aw05, bw0j, \p8w, \p5w
    MADD_V3 \p8w, \p6w, aw06, bw0j, \p8w, \p6w
    MADD_V3 \p8w, \p7w, aw07, bw0j, \p8w, \p7w
.endm


///////////////////////////////////////////////////////////////////////////////
//////// HIGH-LEVEL MACROS FOR OPERAND-SCANNING MODULAR MULTIPLICATION ////////
///////////////////////////////////////////////////////////////////////////////


// The macro `MULROWS` multiplies the 8-word operand $a$ (i.e., the eight words
// `aw00`-`aw07`) by the 8-word operand $b$ (which is loaded word by word from
// RAM) in a row-wise fashion. The 16-word product is put in `rw00`-`rw15`.

.macro MULROWS
    MROW_V1 rw08, rw07, rw06, rw05, rw04, rw03, rw02, rw01, rw00, 0
    MROW_V2 rw09, rw08, rw07, rw06, rw05, rw04, rw03, rw02, rw01, 4
    MROW_V2 rw10, rw09, rw08, rw07, rw06, rw05, rw04, rw03, rw02, 8
    MROW_V2 rw11, rw10, rw09, rw08, rw07, rw06, rw05, rw04, rw03, 12
    MROW_V2 rw12, rw11, rw10, rw09, rw08, rw07, rw06, rw05, rw04, 16
    MROW_V2 rw13, rw12, rw11, rw10, rw09, rw08, rw07, rw06, rw05, 20
    MROW_V2 rw14, rw13, rw12, rw11, rw10, rw09, rw08, rw07, rw06, 24
    MROW_V2 rw15, rw14, rw13, rw12, rw11, rw10, rw09, rw08, rw07, 28
.endm


// The macro `MODREDP` reduces the 16-word operand `rw00`-`rw15` modulo the
// prime $p = 2^{256} - 19$ to get an 8-word result. This reduction consists
// of two steps: (i) the upper half `rw08`-`rw15` is multiplied by $2c = 38$
// and the obtained 9-word product is added to the lower half `rw08`-`rw15`,
// yielding a 9-word intermediate result `rw00`-`rw08`, (ii) the double-length
// word in the `rw08:rw07` register-pair is split into a lower part of 31 bits
// and an upper part, which is multiplied by $c = 19$ and the obtained product
// is added to the intermediate result `rw00`-`rw07`. The implementation below
// merges these steps and performs the modular reduction as follows: (i) `rw15`
// is multiplied by $2c$ and `rw07` is added to the product (macro `MADD_V2`),
// (ii) the obtained product is split into a 31-bit lower part and a (smaller)
// upper part, which is multiplied by $c = 19$ (macro `MULHIXC`), (iii) the
// obtained single-word product is taken into account when the remaining words
// `rw08`-`rw14` are multiplied by $2c$ and added to the words `rw00`-`rw06`.

.macro MODREDP
    li      rcon, 2*CONSTC  // rcon = 38
    MADD_V2 tmp1, rw07, rw15, rcon, rw07
    MULHIXC tmp1, rw07, tmp1, rw07
    MADD_V3 tmp1, rw00, rw08, rcon, tmp1, rw00
    MADD_V3 tmp1, rw01, rw09, rcon, tmp1, rw01
    MADD_V3 tmp1, rw02, rw10, rcon, tmp1, rw02
    MADD_V3 tmp1, rw03, rw11, rcon, tmp1, rw03
    MADD_V3 tmp1, rw04, rw12, rcon, tmp1, rw04
    MADD_V3 tmp1, rw05, rw13, rcon, tmp1, rw05
    MADD_V3 tmp1, rw06, rw14, rcon, tmp1, rw06
    add     rw07, rw07, tmp1
.endm


///////////////////////////////////////////////////////////////////////////////
////////// HELPER MACROS FOR OPERAND-SCANNING MODULAR MULTIPLICATION //////////
///////////////////////////////////////////////////////////////////////////////


// The macro `PROLOGUE` pushes all 12 callee-saved registers on the stack.

.macro PROLOGUE
    addi    sp, sp, -48
    sw      s0, (sp)
    sw      s1, 4(sp)
    sw      s2, 8(sp)
    sw      s3, 12(sp)
    sw      s4, 16(sp)
    sw      s5, 20(sp)
    sw      s6, 24(sp)
    sw      s7, 28(sp)
    sw      s8, 32(sp)
    sw      s9, 36(sp)
    sw      s10, 40(sp)
    sw      s11, 44(sp)
.endm


// The macro `EPILOGUE` pops all 12 callee-saved registers from the stack.

.macro EPILOGUE
    lw      s0, (sp)
    lw      s1, 4(sp)
    lw      s2, 8(sp)
    lw      s3, 12(sp)
    lw      s4, 16(sp)
    lw      s5, 20(sp)
    lw      s6, 24(sp)
    lw      s7, 28(sp)
    lw      s8, 32(sp)
    lw      s9, 36(sp)
    lw      s10, 40(sp)
    lw      s11, 44(sp)
    addi    sp, sp, 48
.endm


// The macro `LDM_OPA` loads the eight words of array `a` from RAM and puts
// them in registers `aw00`-`aw07`.

.macro LDM_OPA
    lw      aw00, 0(aptr)
    lw      aw01, 4(aptr)
    lw      aw02, 8(aptr)
    lw      aw03, 12(aptr)
    lw      aw04, 16(aptr)
    lw      aw05, 20(aptr)
    lw      aw06, 24(aptr)
    lw      aw07, 28(aptr)
.endm


// The macro `STM_RES` stores the eight result-words, which are in registers
// `rw00`-`rw07`, to array `r` in RAM.

.macro STM_RES
    sw      rw00, 0(rptr)
    sw      rw01, 4(rptr)
    sw      rw02, 8(rptr)
    sw      rw03, 12(rptr)
    sw      rw04, 16(rptr)
    sw      rw05, 20(rptr)
    sw      rw06, 24(rptr)
    sw      rw07, 28(rptr)
.endm


///////////////////////////////////////////////////////////////////////////////
///////// SPEED-OPTIMIZED PRIME-FIELD MULTIPLICATION (FULLY UNROLLED) /////////
///////////////////////////////////////////////////////////////////////////////


// This implementation of prime-field multiplication is based on the operand-
// scanning method and includes a fast reduction modulo $p = 2^{255} - 19$. It
// is aimed at high speed, which means the loops are fully unrolled. Each word
// of the arrays `a` and `b` is loaded from RAM exactly once, and each word of
// the result-array `r` is stored to RAM exactly once.

.text
.global gfp_mul_asm
.type gfp_mul_asm,%function
// .balign 8
gfp_mul_asm:
    PROLOGUE            // push callee-saved registers on stack
    LDM_OPA             // load the eight words of array `a` from RAM
    MULROWS             // row-wise multiplication R += A*b[j]*2^(32*j)
    MODREDP             // modular reduction: Rlo = (Rlo + Rhi*2*c) mod p
    STM_RES             // store the eight result-words in array `r` in RAM
    EPILOGUE            // pop callee-saved registers from stack
    ret


.end
