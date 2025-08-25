///////////////////////////////////////////////////////////////////////////////
// mpiarith.c: Basic functions for Multi-Precision Integer (MPI) arithmetic. //
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


// A Multi-Precision Integer (MPI) is represented by an array of type `Word`,
// which is defined in `config.h` as an unsigned 32-bit integer (`uint32_t`),
// whereby the word with index `0` is the least-significant one. Most of the
// MPI functions below are not really performance-critical; therefore, their
// implementation emphasizes flexibility over speed. This means, for example,
// that the length of the arrays is not fixed but can be passed via parameter
// `len` to the function. Each function is written such that it can process
// MPIs of arbitrary length, in steps of `WSIZE` bits.


#include <assert.h>  // assert()
#include <stdio.h>   // printf()
#include <string.h>  // strlen()
#include "mpiarith.h"


// Minimum of two integers
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
// Uppercase letter -> lowercase letter
#define TOLOWER(a) (((a) <= 'Z') ? ((a) + 32) : (a)) 


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


// Some utility functions (e.g., the printing fuction `mpi_print`) use arrays
// allocated on the stack as temporary storage for intermediate results. Since
// these functions support operands of varying length (specified by the `len`
// parameter), the actual array-length can be determined only at run-time. The
// preprocessor statements below set the length of these arrays to either the
// value of the `len` parameter, making these arrays to Variable-Length Arrays
// (VLAs), or twice the `LEN` value defined in `config.h`.
// NOTE: Support of VLAs is a conditional feature of many C compilers and may
// need to be specifically enabled. For example, in IAR Embedded Workbench for
// MSP430, VLAs have to be activated in C/C++ Compiler Options -> "Allow VLA".

#ifdef M25519_USE_VLA
#define _len len  // length parameter
#else  // temp arrays 
#define _len (2*LEN)
#endif


///////////////////////////////////////////////////////////////////////////////
/////// UTILITY FUNCTIONS: INITIALIZATION, COMPARISON, COPYING, PRINTING //////
///////////////////////////////////////////////////////////////////////////////


// Conversion of a hex-string to an MPI
// ------------------------------------

int mpi_from_hex(Word *r, const char *hexstr, int len)
{
  Word w = 0;
  size_t hlen, i = 0, j, m;
  unsigned char c;
  
  // some format checks
  if (hexstr == NULL) return M25519_ERR_HEXSTR;
  if ((hlen = strlen(hexstr)) < 3) return M25519_ERR_HEXSTR;
  if ((hexstr[0] != '0') || (TOLOWER(hexstr[1]) != 'x'))
    return M25519_ERR_HEXSTR;
  
  while ((hlen > 2) && (i < len)) {
    m = MIN(hlen - 2, 2*sizeof(Word));
    for (j = hlen - m; j < hlen; j++) {
      c = (unsigned char) hexstr[j];
      // the 3 LSBs of 'a' and 'A' are 0b001
      c = (c <= '9') ? c - '0' : (c & 7) + 9;
      w = (w << 4) | c;
    }
    r[i++] = w;
    hlen -= m;
    w = 0;
  }
  
  while (i < len) r[i++] = 0;
  
  return M25519_NO_ERROR;
}


// Conversion of an MPI to a hex-string
// ------------------------------------

void mpi_to_hex(char *hexstr, const Word *a, int len)
{
  Word w;
  size_t hlen, i = 0, j;
  unsigned char c;

  hexstr[0] = '0';
  hexstr[1] = 'x';
  hlen = 1 + len*(sizeof(Word) << 1);
  hexstr[hlen+1] = '\0';
  
  while ((hlen >= 2) && (i < len)) {
    w = a[i++];  // from a[0] to a[len-1]
    for (j = 0; j < 2*sizeof(Word); j++) {
      c = ((unsigned char) w) & 0xf;
      // replace 55 by 87 for lowercase letters
      hexstr[hlen-j] = c + ((c < 10) ? 48 : 55);
      w >>= 4;  // go to next nibble
    }
    hlen -= j;
  }
}


// Printing of an MPI as hex-string to `stdout`
// --------------------------------------------

void mpi_print(const char *prefix, const Word *a, int len)
{
  char hexstr[(WSIZE/4)*_len+3];
  
  if ((prefix != NULL) && (strlen(prefix) > 0)) printf("%s", prefix);
  mpi_to_hex(hexstr, a, len);
  printf("%s", hexstr);
  printf("\n");
}


// Initialization of an MPI with a word: $r = [0, \ldots, 0, a]$
// -------------------------------------------------------------

void mpi_setw(Word *r, Word a, int len)
{
  int i;
  
  for (i = len - 1; i > 0; i--) r[i] = 0;
  r[0] = a;
}


// Comparison of an MPI with a word: $a \stackrel{?}{=} [0, \ldots, 0, b]$
// -----------------------------------------------------------------------

int mpi_cmpw(const Word *a, Word b, int len)
{
  Word is0 = 0;
  int i, r = 0;
  
  for (i = len - 1; i > 0; i--) is0 |= a[i];
  r += ((b > a[0]) | (is0 != 0));
  r -= ((b < a[0]) & (is0 == 0));
  
  return r;  // r = 0 if a equals [0,..,0,b]
}


// Comparison of two MPIs: $a \stackrel{?}{=} b$
// ---------------------------------------------

int mpi_cmp(const Word *a, const Word *b, int len)
{
  Word lt = 0, gt = 0;  // less-than, greater-than
  int i, r = 0;
  
  assert(len <= WSIZE);
  
  for (i = len - 1; i >= 0; i--) {
    lt = (lt << 1) | (a[i] < b[i]);
    gt = (gt << 1) | (a[i] > b[i]);
  }
  r += (gt > lt);  // r = +1 if a is greater than b
  r -= (lt > gt);  // r = -1 of a is less than b
  
  return r;  // r = 0 if a equals b
}


// Copying an MPI: $r = a$
// -----------------------

void mpi_copy(Word *r, const Word *a, int len)
{
  int i;
  
  for (i = len - 1; i >= 0; i--) r[i] = a[i];
}


///////////////////////////////////////////////////////////////////////////////
////////////// MULTI-PRECISION ARITHMETIC AND LOGICAL OPERATIONS //////////////
///////////////////////////////////////////////////////////////////////////////


// Addition of two MPIs: $r = a + b$
// ---------------------------------

int mpi_add(Word *r, const Word *a, const Word *b, int len)
{
  DWord sum = 0;
  int i;
  
  for (i = 0; i < len; i++) {
    sum += (DWord) a[i] + b[i];
    r[i] = (Word) sum;
    sum >>= WSIZE;
  }
  
  return (int) sum;  // carry bit
}


// Conditional addition of an MPI to another MPI: $r = a + b$ or $r = a$
// ---------------------------------------------------------------------

int mpi_cadd(Word *r, const Word *a, const Word *b, int add, int len)
{
  DWord sum = 0;
  Word mask = 0 - (add & 1);  // 0 or all-1
  int i;
  
  for (i = 0; i < len; i++) {
    sum += (DWord) a[i] + (b[i] & mask);
    r[i] = (Word) sum;
    sum >>= WSIZE;
  }
  
  return (int) sum;  // carry bit
}


// Multiplication of two MPIs: $r = a \times b$
// --------------------------------------------

void mpi_mul(Word *r, const Word *a, const Word *b, int len)
{
  DWord prod = 0;
  int i, j;
  
  assert((r != a) && (r != b));
  
  // multiplication of A by b[0]
  for (j = 0; j < len; j++) {
    prod += (DWord) a[j]*b[0];
    r[j] = (Word) prod;
    prod >>= WSIZE;
  }
  r[j] = (Word) prod;
  
  // multiplication of A by b[i] for 1 <= i < len
  for (i = 1; i < len; i++) {
    prod = 0;
    for (j = 0; j < len; j++) {
      prod += (DWord) a[j]*b[i];
      prod += r[i+j];
      r[i+j] = (Word) prod;
      prod >>= WSIZE;
    }
    r[i+j] = (Word) prod;
  }
}


///////////////////////////////////////////////////////////////////////////////
#if !defined(M25519_ASSEMBLY) // PERFORMANCE-CRITICAL A/L OPERATIONS //////////
///////////////////////////////////////////////////////////////////////////////


// The following functions are performance-critical since they are executed in
// the main loop of the inversion in GF(p) based on the EEA. In addition to the
// C implementations, there exist also highly-optimized Assembly versions of
// these functions (for certain target architectures like AVR, MSP430, ARMv7-M
// or RV32IM).


// 1-bit right-shift of an MPI: $r = a \gg 1$
// ------------------------------------------

int mpi_shr(Word *r, const Word *a, int len)
{
  int i, retval;
  
  retval = a[0] & 1;  // return value
  for (i = 0; i < len - 1; i++) r[i] = (a[i+1] << (WSIZE - 1)) | (a[i] >> 1);
  r[len-1] = a[len-1] >> 1;
  
  return retval;
}


// Subtraction of an MPI from another MPI: $r = a - b$
// ---------------------------------------------------

int mpi_sub(Word *r, const Word *a, const Word *b, int len)
{
  DWord dif = 1;
  int i;
  
  for (i = 0; i < len; i++) {
    dif += (DWord) a[i] + (~b[i]);
    r[i] = (Word) dif;
    dif >>= WSIZE;
  }
  
  return (1 - ((int) dif));  // borrow bit
}


///////////////////////////////////////////////////////////////////////////////
#endif /////////// ADDITIONAL OR ALTERNATIVE IMPLEMENTATIONS //////////////////
///////////////////////////////////////////////////////////////////////////////

/*
// Multiplication of an MPI by a 32-bit value: $r = a \cdot b$
// -----------------------------------------------------------

void mpi_mul32(Word *r, const Word *a, const Word *b, int len)
{
  DWord prod = 0;
  int i;
  
  // multiplication of A by b[0] (to avoid r <- 0)
  for (i = 0; i < len; i++) {
    prod += (DWord) a[i]*b[0];
    r[i] = (Word) prod;
    prod >>= WSIZE;
  }
  r[i] = (Word) prod;
}


// Squaring of an MPI: $r = a^2$
// -----------------------------

void mpi_sqr(Word *r, const Word *a, int len)
{
  DWord prod = 0, sum = 0;
  int i, j;
  
  assert(r != a);
  
  // multiplication of A[1,...,len-1] by a[0] (to avoid r <- 0)
  r[0] = 0;
  for (j = 1; j < len; j++) {
    prod += (DWord) a[j]*a[0];
    r[j] = (Word) prod;
    prod >>= WSIZE;
  }
  r[j] = (Word) prod;
  
  // multiplication of A[i+1,...,len-1] by a[i] for 1 <= i < len
  for (i = 1; i < len; i++) {
    prod = 0;
    for (j = i + 1; j < len; j++) {
      prod += (DWord) a[j]*a[i];
      prod += r[i+j];
      r[i+j] = (Word) prod;
      prod >>= WSIZE;
    }
    r[i+j] = (Word) prod;
  }
  
  // double existing result, add squares a[i]^2 for 0 <= i < len
  for (i = 0; i < len; i++) {
    prod = (DWord) a[i]*a[i];
    sum += (Word) prod;
    sum += (DWord) r[2*i] + r[2*i];
    r[2*i] = (Word) sum;
    sum >>= WSIZE;
    sum += ((Word) (prod >> WSIZE));
    sum += (DWord) r[2*i+1] + r[2*i+1];
    r[2*i+1] = (Word) sum;
    sum >>= WSIZE;
  }
}
*/
