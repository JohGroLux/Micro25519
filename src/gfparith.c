///////////////////////////////////////////////////////////////////////////////
// gfparith.c: Basic arithmetic operations in a 255-bit prime field GF(p).   //
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


// Each arithmetic function below operates in a prime field GF(p) given by a
// pseudo-Mersenne $p = 2^k - c$, namely the prime with $k = 255$ and $c = 19$.
// An element of this field is represented as an array of type `Word`, which is
// defined in `config.h` as an unsigned 32-bit integer (`uint32_t`), whereby
// the word with index `0` is the least-significant one. These arrays have a
// fixed length of eight words and can, therefore, accommodate up to 256 bits.

// Each arithmetic function accepts incompletely reduced operands as inputs,
// i.e., a field-element does not necessarily need to be the least non-negative
// residue modulo $p$. In fact, the functions can handle any input in the range
// $[0, 2^{256}-1]$ properly. The result of an arithmetic function is also not
// necessarily fully reduced but is guaranteed to be in the range $[0, 2p-1]$,
// which means that the least non-negative residue can be obtained by (at most)
// one subtraction of $p$.


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mpiarith.h"
#include "gfparith.h"


// All-1 Mask: 0xFFFFFFFF
#define ALL1MASK ((Word) -1L)
// MSB-1 Mask: 0x80000000
#define MSB1MASK ((Word) (1UL << (WSIZE - 1)))
// MSB-0 Mask: 0x7FFFFFFF
#define MSB0MASK (ALL1MASK >> 1)
// Minus-4 Mask: 0xFFFFFFFC
#define MIN4MASK ((Word) -4L)
// 4*p[LEN-1]: 0x1FFFFFFFC (33 bits long)
#define FOURXPHI (((DWord) MSB0MASK) << 2)
// 4*p[i], 1 <= i < len-1: 0x3FFFFFFFC (34 bits long)
#define FOURXPMI (((DWord) ALL1MASK) << 2)
// 4*p[0]: 0x3FFFFFFB4 (34 bits long)
#define FOURXPLO (((DWord) (ALL1MASK - (CONSTC - 1))) << 2)

// Maximum of two integers
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))


// Starting with ISO C23 (i.e., ISO/IEC 9899:2024), two's complement is the
// standard representation for signed integers. However, older revisions of C
// also allowed signed integers in one's complement and sign-magnitude form,
// respectively. Some of the arithmetic functions in this file are optimized
// for (and, thus, require) two's complement representation. The pre-processor
// statements below check whether the compiler uses the two's complement form
// (which should be the case for virtually all decent C compilers).

#if ((-1) != ((~1) + 1))
#error "Compiler does not represent a signed integer in two's complement form!"
#endif


// The result of a right-shift of a signed integer is not specified by the ISO
// C standard and, therefore, depends on the compiler. However, the majority of
// modern compilers perform a sign-preserving (i.e., arithmetic) right-shift.
// Some of the arithmetic functions in this file are optimized for (and, thus,
// require) the sign to be preserved. The pre-processor statements below check
// whether the right-shift is an arithmetic or a logical shift. In the latter
// case (which should be *very* unlikely), the alternative implementations at
// the bottom of this file can be used.

#if (((-1) >> 1) != (-1))
#error "Compiler does not right-shift signed integers via an arithmetic shift!"
#endif


///////////////////////////////////////////////////////////////////////////////
/////////// SIMPLE (NON-PERFORMANCE-CRITICAL) PRIME-FIELD OPERATIONS //////////
///////////////////////////////////////////////////////////////////////////////


// Initialization of a field-element with p: $r = p$
// -------------------------------------------------
// This function sets a field-element to the prime $p = 2^k - c$.

void gfp_setp(Word *r)
{
  int i;

  r[LEN-1] = MSB0MASK;  // 0x7FFFFFFF
  for (i = LEN - 2; i > 0; i--) r[i] = ALL1MASK;  // 0xFFFFFFFF
  r[0] = (Word) (0 - CONSTC);
}


// Comparison of a field-element with p: $a \stackrel{?}{=} p$
// -----------------------------------------------------------
// This function compares a (potentially incompletely reduced) field-element
// with the prime $p = 2^k - c$. The return value is `1` when $a > p$, `0` when
// $a = p$, or `-1` when $a < p$. The comparison is implemented with the goal
// of having constant (i.e., operand-independent) execution time.

int gfp_cmpp(const Word *a)
{
  Word lt = 0, gt = 0;  // less-than, greater-than
  int i, r = 0;

  // assert(len <= WSIZE);
  
  lt = (a[LEN-1] < MSB0MASK);  // 0x7FFFFFFF
  gt = (a[LEN-1] > MSB0MASK);  // 0x7FFFFFFF
  
  for (i = LEN - 2; i > 0; i--) {
    lt = (lt << 1) | (a[i] < ALL1MASK);  // 0xFFFFFFFF
    gt = (gt << 1);
  }
  lt = (lt << 1) | (a[0] < (Word) (0 - CONSTC));  // 2's comp
  gt = (gt << 1) | (a[0] > (Word) (0 - CONSTC));  // 2's comp
  
  r += (gt > lt);  // r = +1 if a is greater than p
  r -= (lt > gt);  // r = -1 if a is less than p
  
  return r;  // r = 0 if a equals p
}


///////////////////////////////////////////////////////////////////////////////
#if !defined(M25519_ASSEMBLY) // PERFORMANCE-CRITICAL PRIME-FIELD OPERATIONS //
///////////////////////////////////////////////////////////////////////////////


// The following functions are performance-critical since they are executed in
// the main loop of a scalar multiplication algorithm (e.g., Montgomery ladder
// or fixed-base comb method) or the main loop of the inversion in GF(p) based
// on the extended Euclidean algorithm. In addition to the C implementations,
// there exist also highly-optimized Assembly versions of these functions (for
// certain target architectures like AVR, MSP430, ARMv7-M or RV32IM).


// Addition of two field-elements: $r = a + b \bmod p$
// ---------------------------------------------------
// A straightforward approach for addition in GF(p) consists of a conventional
// long-integer addition, followed by a subtraction of $p$ if the sum is bigger
// than or equal to $p$. However, a scalable (or size-optimized) implementation
// of this approach requires executing two loops. To avoid the second loop, an
// implementation has to start the addition at the two most-significant words
// and integrate the reduction modulo $p$ in the addition. More concretely, the
// sum of a[len-1] and b[len-1] is split up into a $WSIZE-1$-bit lower part and
// an upper part that is at most 2 bits long. This upper part is multiplied by
// the constant `c` and the product, which fits in a single word, is included
// in the addition of the two operands.

void gfp_add(Word *r, const Word *a, const Word *b)
{
  DWord sum;
  Word msw;
  int i;

  sum = (DWord) a[LEN-1] + b[LEN-1];
  msw = ((Word) sum) & MSB0MASK;  // 0x7FFFFFFF
  sum = (DWord) CONSTC*((Word) (sum >> (WSIZE - 1)));
  // sum is in [0, 3*c]
  
  for (i = 0; i < LEN - 1; i++) {
    sum += (DWord) a[i] + b[i];
    r[i] = (Word) sum;
    sum >>= WSIZE;
    // sum is in [0, 2]
  }
  r[LEN-1] = msw + ((Word) sum);
}


// Subtraction of one field-element from another: $r = a - b \bmod p$
// ------------------------------------------------------------------
// Similar to the addition in GF(p), the subtraction is executed with a single
// loop. To ensure that the final result is positive, the subtraction in GF(p)
// is implemented as $r = 4p + a - b \bmod p = 2^{k+2} + a - b - 4c \bmod p$,
// i.e., an addition of $4p = 2^{k+2} - 4c$ is included in the subtraction. In
// this way, the subtraction-loop consists of the same number of instructions
// as the loop of the addition in GF(p) if (i) the sum in the loop is a signed
// integer and (ii) the right-shift is an arithmetic right-shift (to preserve
// the sign). The most-significant word of $4p$, i.e., $2^{WSIZE+1}$, is split
// up into $2^{WSIZE+1} - 4$ and $4$. The former part, which is $WSIZE+1$ bits
// long, is included in the subtraction of b[len-1] from a[len-1], while the
// latter part is added after the subtraction-loop (to ensure that a negative
// sum from the loop does not make r[len-1] negative).

void gfp_sub(Word *r, const Word *a, const Word *b)
{
  SDWord sum;  // signed!
  Word msw;
  int i;

  sum = (SDWord) FOURXPHI + a[LEN-1] - b[LEN-1];  // 0x1FFFFFFFC
  msw = ((Word) sum) & MSB0MASK;  // 0x7FFFFFFF
  sum = (SDWord) CONSTC*((Word) (sum >> (WSIZE - 1)));
  sum = sum - (CONSTC << 2);
  // sum is in [-3*c, c]
  
  for (i = 0; i < LEN - 1; i++) {
    sum += (SDWord) a[i] - b[i];
    r[i] = (Word) sum;
    sum >>= WSIZE;  // arithmetic shift!
    // sum is in [-2, 1]
  }
  r[LEN-1] = msw + ((Word) sum) + 4;
  // 0x1FFFFFFFC + 4 = 0x200000000 = MSW of 2^(k+2)
}


// Conditional negation of a field-element: $r = -a \bmod p$ or $r = a \bmod p$
// ----------------------------------------------------------------------------
// The goal is to execute this function with one loop and as few instructions
// per iteration as possible. When operand `neg` is 1, the function computes
// $r = 2p + t + 1 - 2c \bmod p = 2^{k+1} - 2c + t - (2c - 1) \bmod p$, where
// $t$ is the ($k+1$)-bit one's complement (or bit-wise inverse) of $a$, i.e.,
// $t = 2^{k+1} - a - 1$. The result $r = 2^{k+1} + 2^{k+1} - a - 4c \bmod p$
// can be written as $r = 4p - a \bmod p$, which means $a$ gets negated. This
// negation is implemented similarly as the subtraction in GF(p). On the other
// hand, when `neg` is 0, the result $r = 2p + a = 2^{k+1} - 2c + a \bmod p$ is
// computed. These two cases (i.e., `neg` = 1 and `neg` = 0) can be unified by
// XORing a[i] with an "all-1" mask, thereby producing the one's complement of
// `a`, or 0, which leaves operand $a$ unmodified. Furthermore, a conditional
// (i.e., AND-masked) subtraction of $2c - 1$ is included in the computation.

void gfp_cneg(Word *r, const Word *a, int neg)
{
  SDWord sum;  // signed!
  Word msw, mask;
  int i;

  mask = 0 - (Word) (neg & 1);  // 0 or all-1
  sum = (SDWord) MIN4MASK + (mask ^ a[LEN-1]);  // 0xFFFFFFFC
  msw = ((Word) sum) & MSB0MASK;  // 0x7FFFFFFF
  sum = (SDWord) CONSTC*((Word) (sum >> (WSIZE - 1)));
  sum = sum - (CONSTC << 1) - (mask & ((CONSTC << 1) - 1));
  // sum is in [-3*c+1, -c+1] if neg is 1
  // sum is in [-c, c] if neg is 0
  
  for (i = 0; i < LEN - 1; i++) {
    sum += (SDWord) (mask ^ a[i]);
    r[i] = (Word) sum;
    sum >>= WSIZE;  // arithmetic shift!
    // sum is in [-1, 1]
  }
  r[LEN-1] = msw + ((Word) sum) + 4;
}


// Halving of a field-element: $r = a/2 \bmod p$
// ---------------------------------------------
// The halving function performs a conventional 1-bit right-shift when the LSB
// of operand $a$ is 0; otherwise, it first adds the prime $p$ to $a$ to make
// it even and executes the shift thereafter. Similar to the other operations
// in GF(p) described above, the addition of the prime $p$ can be optimized by
// subtracting $c$ from the least-significant word a[i] (which can introduce a
// borrow-bit that has to be propagated) and adding $2^{WSIZE-1}$ to the most-
// significant word a[len-1]. The loop has to compute not only a sum, but also
// maintain a temporary copy of the least-significant word of the previous sum
// (i.e., the sum of the last iteration) to be able to perform the 1-bit right-
// shift. Due to the right-shift, the result will always fit into $len$ words.

void gfp_hlv(Word *r, const Word *a)
{
  SDWord sum;  // signed!
  Word tmp, mask;
  int i;

  // masked addition of prime p to a
  mask = 0 - (a[0] & 1);  // 0 or all-1
  sum = (SDWord) a[0] - (CONSTC & mask);
  tmp = (Word) sum;
  sum >>= WSIZE;  // arithmetic shift!
  // sum is in [-1, 0]
  
  for (i = 1; i < LEN - 1; i++) {
    sum += (SDWord) a[i];
    r[i-1] = (((Word) sum) << (WSIZE - 1)) | (tmp >> 1);
    tmp = (Word) sum;
    sum >>= WSIZE;  // arithmetic shift!
    // sum is in [-1, 0]
  }
  sum += (SDWord) a[LEN-1] + (MSB1MASK & mask);  // 0x80000000
  r[LEN-2] = (((Word) sum) << (WSIZE - 1)) | (tmp >> 1);
  r[LEN-1] = (Word) (sum >> 1);
}


// Multiplication of two field-elements: $r = a \cdot b \bmod p$
// -------------------------------------------------------------
// The multiplication in GF(p) consists of an "ordinary" multiplication of the
// two $len$-word operands, yielding a product of $2len$ words (which is stored
// in a temporary array `t` on the stack), succeeded by a reduction modulo $p$.
// Following the operand-scanning technique, the $len$-word multiplication has
// a nested-loop structure, whereby the very first iteration of the outer loop
// (in which the words a[i] of $a$ are multiplied by word b[0]) is peeled off
// to avoid an explicit initialization of array `t` to 0. The modular reduction
// consists of two steps; the first step involves multiplying the higher $len$
// words of `t` by $2c$ and adding the product to the lower $len$ words of `t`.
// The obtained intermediate result comprises $len+1$ words, of which the most-
// significant word is up to $WSIZE-1$ bits long. Thereafter, the second step
// is similar to the reduction-step of the addition in GF(p).

void gfp_mul(Word *r, const Word *a, const Word *b)
{
  Word t[2*LEN];
  DWord prod = 0;
  Word msw;
  int i, j;

  // multiplication of A by b[0]
  for (j = 0; j < LEN; j++) {
    prod += (DWord) a[j]*b[0];
    t[j] = (Word) prod;
    prod >>= WSIZE;
  }
  t[j] = (Word) prod;
  
  // multiplication of A by b[i] for 1 <= i < LEN
  for (i = 1; i < LEN; i++) {
    prod = 0;
    for (j = 0; j < LEN; j++) {
      prod += (DWord) a[j]*b[i];
      prod += t[i+j];
      t[i+j] = (Word) prod;
      prod >>= WSIZE;
    }
    t[i+j] = (Word) prod;
  }
  
  // first step of modular reduction
  prod = 0;
  for (i = 0; i < LEN - 1; i++) {
    prod += (DWord) t[i+LEN]*(CONSTC << 1) + t[i];
    t[i] = (Word) prod;
    prod >>= WSIZE;
  }
  prod += (DWord) t[2*LEN-1]*(CONSTC << 1) + t[LEN-1];
  // prod is in [0, 2^(2*WSIZE-1)-1]
  
  // second step of modular reduction
  msw = ((Word) prod) & MSB0MASK;  // 0x7FFFFFFF
  prod = (DWord) CONSTC*(prod >> (WSIZE - 1));
  for (i = 0; i < LEN - 1; i++) {
    prod += t[i];
    r[i] = (Word) prod;
    prod >>= WSIZE;
  }
  r[LEN-1] = msw + ((Word) prod);
}


// Squaring of a field-element: $r = a^2 \bmod p$
// ----------------------------------------------
// Similar to the multiplication, the implementation of the squaring is based
// on the operand-scanning method. To minimize the execution time, all partial-
// products a[j]*a[i] with j != i are computed only once and then doubled in a
// separate loop. Compared to the multiplication, the nested loop computes less
// than half of the number of partial products since the loop-counter j of the
// inner loop starts with j = i + 1 instead of j = 0. The very first iteration
// of the outer loop is peeled off to avoid an explicit initialization of array
// `t` (in which the square is stored) to 0. After the nested loop follows the
// doubling of the intermediate result obtained so far along with the addition
// of the (un-doubled) partial-products a[i]*a[i] located in main diagonal. The
// reduction modulo $p$ is performed in the same way as for the multiplication
// in GF(p).

void gfp_sqr(Word *r, const Word *a)
{
  Word t[2*LEN];
  DWord prod = 0, sum = 0;
  Word msw;
  int i, j;

  // multiplication of A[1,...,LEN-1] by a[0] (to avoid r <- 0)
  t[0] = 0;
  for (j = 1; j < LEN; j++) {
    prod += (DWord) a[j]*a[0];
    t[j] = (Word) prod;
    prod >>= WSIZE;
  }
  t[j] = (Word) prod;
  
  // multiplication of A[i+1,...,LEN-1] by a[i] for 1 <= i < LEN
  for (i = 1; i < LEN; i++) {
    prod = 0;
    for (j = i + 1; j < LEN; j++) {
      prod += (DWord) a[j]*a[i];
      prod += t[i+j];
      t[i+j] = (Word) prod;
      prod >>= WSIZE;
    }
    t[i+j] = (Word) prod;
  }
  
  // double existing result, add squares a[i]^2 for 0 <= i < LEN
  for (i = 0; i < LEN; i++) {
    prod = (DWord) a[i]*a[i];
    sum += (Word) prod;
    sum += (DWord) t[2*i] + t[2*i];
    t[2*i] = (Word) sum;
    sum >>= WSIZE;
    sum += ((Word) (prod >> WSIZE));
    sum += (DWord) t[2*i+1] + t[2*i+1];
    t[2*i+1] = (Word) sum;
    sum >>= WSIZE;
  }
  
  // first step of modular reduction
  prod = 0;
  for (i = 0; i < LEN - 1; i++) {
    prod += (DWord) t[i+LEN]*(CONSTC << 1) + t[i];
    t[i] = (Word) prod;
    prod >>= WSIZE;
  }
  prod += (DWord) t[2*LEN-1]*(CONSTC << 1) + t[LEN-1];
  // prod is in [0, 2^(2*WSIZE-1)-1]
  
  // second step of modular reduction
  msw = ((Word) prod) & MSB0MASK;  // 0x7FFFFFFF
  prod = (DWord) CONSTC*(prod >> (WSIZE - 1));
  for (i = 0; i < LEN - 1; i++) {
    prod += t[i];
    r[i] = (Word) prod;
    prod >>= WSIZE;
  }
  r[LEN-1] = msw + ((Word) prod);
}


// Multiplication of a field-element by a 32-bit value: $r = a \cdot b \bmod p$
// ----------------------------------------------------------------------------
// The multiplication of a $len$-word integer by a 32-bit integer follows the
// operand-scanning method and yields a product consisting of $len+32/WSIZE$
// words, which is stored in a temporary array `t` on the stack. The reduction
// operation differs slightly from the reduction of the normal multiplication
// in GF(p) and starts with a multiplication the $32/WSIZE$ higher words of `t`
// by $2c$ and an addition of $c$ to this product if the MSB of t[len-1] is 1.
// Then, the lower $32/WSIZE$ words of the obtained sum (which is $32/WSIZE+1$
// words long) are added to the $32/WSIZE$ least-significant words of `t`. The
// produced carry is added along with the highest sum-word to t[32/WSIZE] and
// the resulting carry is propagated up to the most-significant word t[len-1],
// whose MSB has been cleared before the carry propagation.

void gfp_mul32(Word *r, const Word *a, const Word *b)
{
  Word t[LEN+1];
  DWord prod = 0;
  Word msw;
  int i = 0, j;

  // multiplication of A by b[0]
  for (j = 0; j < LEN; j++) {
    prod += (DWord) a[j]*b[0];
    t[j] = (Word) prod;
    prod >>= WSIZE;
  }
  t[j] = (Word) prod;
  
  msw = t[LEN-1] & MSB0MASK;  // 0x7FFFFFFF
  prod = (DWord) CONSTC*(t[LEN-1] >> (WSIZE - 1));
  // prod is either 0 or c
  
  // compute first 32 bits of result
  prod += (DWord) t[i+LEN]*(CONSTC << 1) + t[i];
  r[i] = (Word) prod;
  prod >>= WSIZE;
  
  // compute r[i] = t[i] + carry
  for (i = 1; i < LEN - 1; i++) {
    prod += (DWord) t[i];
    r[i] = (Word) prod;
    prod >>= WSIZE;
  }
  r[LEN-1] = ((Word) prod) + msw;
}


///////////////////////////////////////////////////////////////////////////////
#endif /////////////// COMPOSITE PRIME-FIELD OPERATIONS ///////////////////////
///////////////////////////////////////////////////////////////////////////////


// The following functions are non-leaf functions, i.e., they are composed of
// other field-arithmetic or MPI functions. Only the field-inversion `gfp_inv`
// is performance-critical.


// Full reduction of a field-element: $r = a \bmod p$
// --------------------------------------------------
// This function computes the least non-negative residue of a field-element by
// subtracting the prime $p$ (via `mpi_sub`) and re-adding it if the difference
// was negative (`mpi_cadd`). Each GF(p) arithmetic function provided in this
// file produces a result in the range $[0, 2p-1]$, i.e., a single subtraction
// of $p$ should be sufficient, but an externally assigned field-element can be
// greater than $2p$. To ensure constant execution time, two subtractions (and
// two conditional re-additions) of $p$ are performed.

void gfp_fred(Word *r, const Word *a)
{
  Word p[LEN];
  int rbit;  // borrow
  
  gfp_setp(p);
  rbit = mpi_sub(r, a, p, LEN);
  mpi_cadd(r, r, p, rbit, LEN);
  rbit = mpi_sub(r, r, p, LEN);
  mpi_cadd(r, r, p, rbit, LEN);
}


// Comparison of two field-elements: $a \stackrel{?}{=} b$
// -------------------------------------------------------
// This function compares two (potentially incompletely reduced) field-elements
// $a$ and $b$. The return value is `1` when $a > b$, `0` when $a = b$, or `-1`
// when $a < b$. Each field-element is first fully reduced via two subtractions
// (and conditional re-additions) of the prime $p$, similar to `gfp_fred`. The
// comparison is implemented with the goal of having constant (i.e., operand-
// independent) execution time.

int gfp_cmp(const Word *a, const Word *b)
{
  Word tmp[3*LEN];  // temporary space for three gfp elements
  Word *p = tmp, *ar = &tmp[LEN], *br = &tmp[2*LEN];
  int rbit;  // borrow
  
  gfp_setp(p);
  // full reduction of a
  rbit = mpi_sub(ar, a, p, LEN);
  mpi_cadd(ar, ar, p, rbit, LEN);
  rbit = mpi_sub(ar, ar, p, LEN);
  mpi_cadd(ar, ar, p, rbit, LEN);
  // full reduction of b
  rbit = mpi_sub(br, b, p, LEN);
  mpi_cadd(br, br, p, rbit, LEN);
  rbit = mpi_sub(br, br, p, LEN);
  mpi_cadd(br, br, p, rbit, LEN);
  // comparison of a and b
  rbit = mpi_cmp(ar, br, LEN);
  
  return rbit;
}


// Inversion of a non-0 field-element: $r = a^{-1} \bmod p$
// --------------------------------------------------------
// This function computes the multiplicative inverse of a non-0 field-element
// modulo the prime $p$ using an optimized variant of the Extended Euclidean
// Algorithm (EEA). One of these optimizations is to keep track of the actual
// length of intermediate results (i.e., `ux` and `vx`) and perform the right-
// shift operations (`mpi_shr`) and subtractions (`mpi_sub`) only with their
// non-0 words, which reduces the total execution time as these intermediate
// results become gradually shorter.
// NOTE: The EEA has an operand-dependent execution pattern and, therefore, an
// operand-dependent execution time. However, `gfp_inv` can be efficiently and
// effectively protected against timing attacks by applying a multiplicative
// masking technique as follows: the field-element $x$ to be inverted is first
// multiplied by a field-element $u$ that is unknown to the attacker, then the
// product $x \cdot u$ is inverted, and finally the inverse $(x \cdot u)^{−1}$
// is multiplied by $u$ to get $x^{−1}$.
// NOTE: The function returns `M25519_ERR_INVERS` if the field-element to be
// inverted is `0` and `M25519_NO_ERROR` otherwise.

int gfp_inv(Word *r, const Word *a)
{
  Word tmp[3*LEN];  // temporary space for three gfp elements
  Word *ux = tmp, *vx = &tmp[LEN], *x1 = &tmp[2*LEN], *x2 = r;
  int uvlen = LEN;
  
  mpi_copy(ux, a, LEN);  // set ux = a
  gfp_setp(vx);          // set vx = p
  mpi_setw(x1, 1, LEN);  // set x1 = 1
  mpi_setw(x2, 0, LEN);  // set x2 = 0
  
  while (mpi_cmp(ux, vx, LEN) >= 0) mpi_sub(ux, ux, vx, LEN);
  if (mpi_cmpw(ux, 0, LEN) == 0) return M25519_ERR_INVERS;
  
  while(mpi_cmpw(ux, 1, uvlen) && mpi_cmpw(vx, 1, uvlen)) {
    while((ux[0] & 1) == 0) {  // ux is even
      mpi_shr(ux, ux, uvlen);
      gfp_hlv(x1, x1);
    }
    while((vx[0] & 1) == 0) {  // vx is even
      mpi_shr(vx, vx, uvlen);
      gfp_hlv(x2, x2);
    }
    // now both ux and vx are odd
    if (mpi_cmp(ux, vx, uvlen) >= 0) {
      mpi_sub(ux, ux, vx, uvlen);
      gfp_sub(x1, x1, x2);
    } else {
      mpi_sub(vx, vx, ux, uvlen);
      gfp_sub(x2, x2, x1);
    }
    if ((ux[uvlen-1] == 0) && (vx[uvlen-1] == 0)) uvlen--;
  }
  
  if (mpi_cmpw(ux, 1, LEN) == 0) mpi_copy(r, x1, LEN);
  return M25519_NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
////////////////// ADDITIONAL OR ALTERNATIVE IMPLEMENTATIONS //////////////////
///////////////////////////////////////////////////////////////////////////////

/*
// Subtraction of one field-element from another: $r = a - b \bmod p$
// ------------------------------------------------------------------
// The optimized `gfp_sub` function from above requires the right-shift of the
// signed integer `sum` (inside the loop) to be an arithmetic shift so that its
// sign is preserved. However, according to the ISO C23 standard, the result of
// a right-shift depends on the compiler (resp., architecture) when the shifted
// operand is of a signed type and has a negative value. The function below is
// an alternative implementation of `gfp_sub` that uses an unsigned `sum` and,
// thus, avoids shifting a signed type. The modified loop is a bit slower, but
// more portable, than the original `gfp_sub`.

void gfp_sub_v2(Word *r, const Word *a, const Word *b)
{
  DWord sum;
  Word msw;
  int i;
  
  sum = (DWord) FOURXPHI + a[LEN-1] - b[LEN-1];  // 0x1FFFFFFFC
  msw = ((Word) sum) & MSB0MASK;  // 0x7FFFFFFF
  sum = (DWord) CONSTC*((Word) (sum >> (WSIZE - 1)));

  sum += (DWord) FOURXPLO + a[0] - b[0];  // 0x3FFFFFFB4
  r[0] = (Word) sum;
  sum >>= WSIZE;
  
  for (i = 1; i < LEN - 1; i++) {
    sum += (DWord) FOURXPMI + a[i] - b[i];  // 0x3FFFFFFFC
    r[i] = (Word) sum;
    sum >>= WSIZE;
  }
  r[LEN-1] = msw + ((Word) sum);
}


// Conditional negation of a field-element: $r = -a \bmod p$ or $r = a \bmod p$
// ----------------------------------------------------------------------------
// The optimized `gfp_cneg` function from above requires the right-shift of the
// signed integer `sum` (inside the loop) to be an arithmetic shift so that its
// sign is preserved. However, according to the ISO C23 standard, the result of
// a right-shift depends on the compiler (resp., architecture) when the shifted
// operand is of a signed type and has a negative value. The function below is
// an alternative implementation of `gfp_cneg` that uses an unsigned `sum` and,
// thus, avoids shifting a signed type. The modified loop is a bit slower, but
// more portable, than the original `gfp_cneg`.

void gfp_cneg_v2(Word *r, const Word *a, int neg)
{
  DWord sum;
  Word msw, mask;
  int i;

  mask = 0 - (Word) (neg & 1);  // 0 or all-1
  sum = (DWord) (FOURXPHI >> 1) + (mask ^ a[LEN-1]);  // 0xFFFFFFFE
  msw = ((Word) sum) & MSB0MASK;  // 0x7FFFFFFF
  sum = (DWord) CONSTC*((Word) (sum >> (WSIZE - 1)));
  sum = sum - (mask & ((CONSTC << 1) - 1));

  sum += (DWord) (FOURXPLO >> 1) + (mask ^ a[0]);  // 0x1FFFFFFDA
  r[0] = (Word) sum;
  sum >>= WSIZE;

  for (i = 1; i < LEN - 1; i++) {
    sum += (DWord) (FOURXPMI >> 1) + (mask ^ a[i]);  // 0x1FFFFFFFE
    r[i] = (Word) sum;
    sum >>= WSIZE;
  }
  r[LEN-1] = msw + ((Word) sum);
}


// Halving of a field-element: $r = a/2 \bmod p$
// ---------------------------------------------
// The optimized `gfp_hlv` function from above requires the right-shift of the
// signed integer `sum` (inside the loop) to be an arithmetic shift so that its
// sign is preserved. However, according to the ISO C23 standard, the result of
// a right-shift depends on the compiler (resp., architecture) when the shifted
// operand is of a signed type and has a negative value. The function below is
// an alternative implementation of `gfp_hlv` that uses an unsigned `sum` and,
// thus, avoids shifting a signed type. The modified loop is a bit slower, but
// more portable, than the original `gfp_hlv`.

void gfp_hlv_v2(Word *r, const Word *a)
{
  DWord sum;
  Word tmp, mask;
  int i;
  
  // masked addition of prime p to a
  mask = 0 - (a[0] & 1);  // 0 or all-1
  sum = (DWord) a[0] + ((0 - CONSTC) & mask);
  tmp = (Word) sum;
  sum >>= WSIZE;
  
  for (i = 1; i < LEN - 1; i++) {
    sum += (DWord) a[i] + mask;
    r[i-1] = (((Word) sum) << (WSIZE - 1)) | (tmp >> 1);
    tmp = (Word) sum;
    sum >>= WSIZE;
  }
  sum += (DWord) a[LEN-1] + (MSB0MASK & mask);  // 0x7FFFFFFF
  r[LEN-2] = (((Word) sum) << (WSIZE - 1)) | (tmp >> 1);
  r[LEN-1] = (Word) (sum >> 1);
}


// Reduction of a product of two field-elements: $r = a \bmod p$
// -------------------------------------------------------------
// This function reduces a product of two field-elements, which has a length of
// $2len$ words, modulo the pseudo-Mersenne prime $p = 2^k - c$. Such a modular
// reduction is included in `gfp_mul` above. The modular reduction consists of
// two steps; the first step involves multiplying the higher $len$ words of `a`
// by $2c$ and adding the product to the lower $len$ words of `a`. The obtained
// intermediate result comprises $len+1$ words, of which the most-significant
// word is up to $WSIZE-1$ bits long. Thereafter, the second step is similar to
// the reduction-step of the addition in GF(p).

void gfp_red(Word *r, const Word *a)
{
  DWord sum, prod = 0;
  Word msw;
  int i;
  
  // first step of modular reduction
  for (i = 0; i < LEN - 1; i++) {
    prod += (DWord) a[i+LEN]*(CONSTC << 1) + a[i];
    r[i] = (Word) prod;
    prod >>= WSIZE;
  }
  prod += (DWord) a[2*LEN-1]*(CONSTC << 1) + a[LEN-1];
  // prod is in [0, 2^(2*WSIZE-1)-1]
  
  // second step of modular reduction
  msw = ((Word) prod) & MSB0MASK;  // 0x7FFFFFFF
  sum = (DWord) CONSTC*(prod >> (WSIZE - 1));
  for (i = 0; i < LEN - 1; i++) {
    sum += r[i];
    r[i] = (Word) sum;
    sum >>= WSIZE;
  }
  r[LEN-1] = msw + ((Word) sum);
}


// Reduction of a product of a field-element by a 32-bit value: $r = a \bmod p$
// ----------------------------------------------------------------------------
// This function reduces a product of a field-element and a 32-bit value, which
// has a length of $len+32/WSIZE$ words, modulo the pseudo-Mersenne prime $p =
// 2^k - c$. Such a modular reduction is included in `gfp_mul32` above. This
// reduction differs slightly from the reduction of the normal multiplication
// in GF(p) and starts with a multiplication the $32/WSIZE$ higher words of `a`
// by $2c$ and an addition of $c$ to this product if the MSB of a[len-1] is 1.
// Then, the lower $32/WSIZE$ words of the obtained sum (which is $32/WSIZE+1$
// words long) are added to the $32/WSIZE$ least-significant words of `a`. The
// produced carry is added along with the highest sum-word to a[32/WSIZE] and
// the resulting carry is propagated up to the most-significant word a[len-1],
// whose MSB has been cleared before the carry propagation.

void gfp_red32(Word *r, const Word *a)
{
  DWord prod;
  Word msw;
  int i;
  
  prod = (DWord) CONSTC*(a[LEN-1] >> (WSIZE - 1));
  msw = a[LEN-1] & MSB0MASK;  // 0x7FFFFFFF
  
  // compute first 32 bits of result
  prod += (DWord) a[LEN]*(CONSTC << 1) + a[0];
  r[0] = (Word) prod;
  prod >>= WSIZE;
  
  // compute r[i] = a[i] + carry
  for (i = 1; i < LEN - 1; i++) {
    prod += (DWord) a[i];
    r[i] = (Word) prod;
    prod >>= WSIZE;
  }
  r[LEN-1] = ((Word) prod) + msw;
}
*/
