///////////////////////////////////////////////////////////////////////////////
// test_gfp_c99.c: Unit tests for C99 implementations of GF(p) arithmetic.   //
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


#include <stdio.h>
#include <string.h>
#include "../src/mpiarith.h"
#include "../src/gfparith.h"


// Length of a line to be read from test-vector file

#define MAXLINE 100


static char *get_vector(char *buffer, FILE *tvfile)
{
  char *op1 = &(buffer[MAXLINE]);
  char *op2 = &(buffer[2*MAXLINE]);
  char *exp = &(buffer[3*MAXLINE]);
  char *rval;  // for error checks
  int i;
  
  op1[0] = op2[0] = exp[0] = '\0';
  
  for (i = 0; i < 3; i++) {
    rval = fgets(buffer, MAXLINE, tvfile);
    if (rval == NULL) return NULL;
    buffer[strcspn(buffer, "\r\n")] = '\0';
    if (strstr(buffer, "op1:") != NULL) {
      strcpy(op1, &(buffer[5]));
      continue;
    }
    if (strstr(buffer, "op2:") != NULL) {
      strcpy(op2, &(buffer[5]));
      continue;
    }
    if (strstr(buffer, "res:") != NULL) {
      strcpy(exp, &(buffer[5]));
      return buffer;
    }
  }
  return buffer;
}


static int chk_vector(const char *op1, const char *op2, const char *exp, \
  Word *res)
{
  char buf[MAXLINE];
  int wrong = 0;
  
  gfp_fred(res, res);
  mpi_to_hex(buf, res, LEN);
  
  if (strcmp(exp, buf) != 0) {
    printf("Testvector verification failed !!!\n");
    if (op1 != NULL) printf("Operand #1: %s\n", op1);
    if (op2 != NULL) printf("Operand #2: %s\n", op2);
    printf("Exp Result: %s\n", exp);
    printf("Act Result: %s\n", buf);
    wrong = 1;
  }
  
  return wrong;
}


int test_gfp_add(const char *tvname)
{
  FILE *tvfile;
  Word op1[LEN], op2[LEN], res[LEN];
  int numtv = 0, wrongtv = 0;
  char buffer[4*MAXLINE];
  char *o1c = &(buffer[MAXLINE]);
  char *o2c = &(buffer[2*MAXLINE]);
  char *exp = &(buffer[3*MAXLINE]);
  char *rval;  // for error checks
  
  tvfile = fopen(tvname, "r");
  if (tvfile == NULL) {
    printf("Test-vector file %s can not be openend!\n", tvname);
    return M25519_ERR_TVFILE;
  }
  printf("Testing gfp_add() with test-vector file %s ...\n", tvname);
  
  buffer[4*MAXLINE-1] = '\0';
  rval = fgets(buffer, MAXLINE, tvfile);
  if (rval == NULL) return M25519_ERR_TVFILE;
  buffer[strcspn(buffer, "\r\n")] = '\0';
  rval = strstr(buffer, "Addition");
  if (rval == NULL) printf("Incorrect test-vector file!\n");
  
  while (rval != NULL) {
    // get next testvector from tv-file
    rval = get_vector(buffer, tvfile);
    if (rval == NULL) break;
    // extract operands from testvector
    mpi_from_hex(op1, &(buffer[1*MAXLINE]), LEN);
    mpi_from_hex(op2, &(buffer[2*MAXLINE]), LEN);
    // execute the arithmetic operation
    gfp_add(res, op1, op2);
    // check result and report mismatch
    wrongtv += chk_vector(o1c, o2c, exp, res);
    numtv++;
  }
  fclose(tvfile);
  
  printf(" -> %i test-vectors verified, ", numtv);
  printf("%i test-vectors wrong\n", wrongtv);
  return numtv;
}


int test_gfp_sub(const char *tvname)
{
  FILE *tvfile;
  Word op1[LEN], op2[LEN], res[LEN];
  int numtv = 0, wrongtv = 0;
  char buffer[4*MAXLINE];
  char *o1c = &(buffer[MAXLINE]);
  char *o2c = &(buffer[2*MAXLINE]);
  char *exp = &(buffer[3*MAXLINE]);
  char *rval;  // for error checks
  
  tvfile = fopen(tvname, "r");
  if (tvfile == NULL) {
    printf("Test-vector file %s can not be openend!\n", tvname);
    return M25519_ERR_TVFILE;
  }
  printf("Testing gfp_sub() with test-vector file %s ...\n", tvname);
  
  buffer[4*MAXLINE-1] = '\0';
  rval = fgets(buffer, MAXLINE, tvfile);
  if (rval == NULL) return M25519_ERR_TVFILE;
  buffer[strcspn(buffer, "\r\n")] = '\0';
  rval = strstr(buffer, "Subtraction");
  if (rval == NULL) printf("Incorrect test-vector file!\n");

  while (rval != NULL) {
    // get next testvector from tv-file
    rval = get_vector(buffer, tvfile);
    if (rval == NULL) break;
    // extract operands from testvector
    mpi_from_hex(op1, &(buffer[1*MAXLINE]), LEN);
    mpi_from_hex(op2, &(buffer[2*MAXLINE]), LEN);
    // execute the arithmetic operation
    gfp_sub(res, op1, op2);
    // check result and report mismatch
    wrongtv += chk_vector(o1c, o2c, exp, res);
    numtv++;
  }
  fclose(tvfile);
  
  printf(" -> %i test-vectors verified, ", numtv);
  printf("%i test-vectors wrong\n", wrongtv);
  return numtv;
}


int test_gfp_mul(const char *tvname)
{
  FILE *tvfile;
  Word op1[LEN], op2[LEN], res[LEN];
  int numtv = 0, wrongtv = 0;
  char buffer[4*MAXLINE];
  char *o1c = &(buffer[MAXLINE]);
  char *o2c = &(buffer[2*MAXLINE]);
  char *exp = &(buffer[3*MAXLINE]);
  char *rval;  // for error checks
  
  tvfile = fopen(tvname, "r");
  if (tvfile == NULL) {
    printf("Test-vector file %s can not be openend!\n", tvname);
    return M25519_ERR_TVFILE;
  }
  printf("Testing gfp_mul() with test-vector file %s ...\n", tvname);
  
  buffer[4*MAXLINE-1] = '\0';
  rval = fgets(buffer, MAXLINE, tvfile);
  if (rval == NULL) return M25519_ERR_TVFILE;
  buffer[strcspn(buffer, "\r\n")] = '\0';
  rval = strstr(buffer, "Multiplication");
  if (rval == NULL) printf("Incorrect test-vector file!\n");

  while (rval != NULL) {
    // get next testvector from tv-file
    rval = get_vector(buffer, tvfile);
    if (rval == NULL) break;
    // extract operands from testvector
    mpi_from_hex(op1, &(buffer[1*MAXLINE]), LEN);
    mpi_from_hex(op2, &(buffer[2*MAXLINE]), LEN);
    // execute the arithmetic operation
    gfp_mul(res, op1, op2);
    // check result and report mismatch
    wrongtv += chk_vector(o1c, o2c, exp, res);
    numtv++;
  }
  fclose(tvfile);
  
  printf(" -> %i test-vectors verified, ", numtv);
  printf("%i test-vectors wrong\n", wrongtv);
  return numtv;
}


int test_gfp_mul32(const char *tvname)
{
  FILE *tvfile;
  Word op1[LEN], op2[1] = { 121666 }, res[LEN];
  int numtv = 0, wrongtv = 0;
  char buffer[4*MAXLINE];
  char *o1c = &(buffer[MAXLINE]);
  char *exp = &(buffer[3*MAXLINE]);
  char *rval;  // for error checks
  
  tvfile = fopen(tvname, "r");
  if (tvfile == NULL) {
    printf("Test-vector file %s can not be openend!\n", tvname);
    return M25519_ERR_TVFILE;
  }
  printf("Testing gfp_mul32() with test-vector file %s ...\n", tvname);
  
  buffer[4*MAXLINE-1] = '\0';
  rval = fgets(buffer, MAXLINE, tvfile);
  if (rval == NULL) return M25519_ERR_TVFILE;
  buffer[strcspn(buffer, "\r\n")] = '\0';
  rval = strstr(buffer, "Multiplication (32 bit)");
  if (rval == NULL) printf("Incorrect test-vector file!\n");

  while (rval != NULL) {
    // get next testvector from tv-file
    rval = get_vector(buffer, tvfile);
    if (rval == NULL) break;
    // extract operands from testvector
    mpi_from_hex(op1, &(buffer[1*MAXLINE]), LEN);
    // execute the arithmetic operation
    gfp_mul32(res, op1, op2);
    // check result and report mismatch
    wrongtv += chk_vector(o1c, NULL, exp, res);
    numtv++;
  }
  fclose(tvfile);
  
  printf(" -> %i test-vectors verified, ", numtv);
  printf("%i test-vectors wrong\n", wrongtv);
  return numtv;
}


int test_gfp_sqr(const char *tvname)
{
  FILE *tvfile;
  Word op1[LEN], res[LEN];
  int numtv = 0, wrongtv = 0;
  char buffer[4*MAXLINE];
  char *o1c = &(buffer[MAXLINE]);
  char *exp = &(buffer[3*MAXLINE]);
  char *rval;  // for error checks
  
  tvfile = fopen(tvname, "r");
  if (tvfile == NULL) {
    printf("Test-vector file %s can not be openend!\n", tvname);
    return M25519_ERR_TVFILE;
  }
  printf("Testing gfp_sqr() with test-vector file %s ...\n", tvname);
  
  buffer[4*MAXLINE-1] = '\0';
  rval = fgets(buffer, MAXLINE, tvfile);
  if (rval == NULL) return M25519_ERR_TVFILE;
  buffer[strcspn(buffer, "\r\n")] = '\0';
  rval = strstr(buffer, "Squaring");
  if (rval == NULL) printf("Incorrect test-vector file!\n");

  while (rval != NULL) {
    // get next testvector from tv-file
    rval = get_vector(buffer, tvfile);
    if (rval == NULL) break;
    // extract operands from testvector
    mpi_from_hex(op1, &(buffer[1*MAXLINE]), LEN);
    // execute the arithmetic operation
    gfp_sqr(res, op1);
    // check result and report mismatch
    wrongtv += chk_vector(o1c, NULL, exp, res);
    numtv++;
  }
  fclose(tvfile);
  
  printf(" -> %i test-vectors verified, ", numtv);
  printf("%i test-vectors wrong\n", wrongtv);
  return numtv;
}


int test_gfp_hlv(const char *tvname)
{
  FILE *tvfile;
  Word op1[LEN], res[LEN];
  int numtv = 0, wrongtv = 0;
  char buffer[4*MAXLINE];
  char *o1c = &(buffer[MAXLINE]);
  char *exp = &(buffer[3*MAXLINE]);
  char *rval;  // for error checks
  
  tvfile = fopen(tvname, "r");
  if (tvfile == NULL) {
    printf("Test-vector file %s can not be openend!\n", tvname);
    return M25519_ERR_TVFILE;
  }
  printf("Testing gfp_hlv() with test-vector file %s ...\n", tvname);
  
  buffer[4*MAXLINE-1] = '\0';
  rval = fgets(buffer, MAXLINE, tvfile);
  if (rval == NULL) return M25519_ERR_TVFILE;
  buffer[strcspn(buffer, "\r\n")] = '\0';
  rval = strstr(buffer, "Halving");
  if (rval == NULL) printf("Incorrect test-vector file!\n");

  while (rval != NULL) {
    // get next testvector from tv-file
    rval = get_vector(buffer, tvfile);
    if (rval == NULL) break;
    // extract operands from testvector
    mpi_from_hex(op1, &(buffer[1*MAXLINE]), LEN);
    // execute the arithmetic operation
    gfp_hlv(res, op1);
    // check result and report mismatch
    wrongtv += chk_vector(o1c, NULL, exp, res);
    numtv++;
  }
  fclose(tvfile);
  
  printf(" -> %i test-vectors verified, ", numtv);
  printf("%i test-vectors wrong\n", wrongtv);
  return numtv;
}


int test_gfp_cneg(const char *tvname)
{
  FILE *tvfile;
  Word op1[LEN], res[LEN];
  int numtv = 0, wrongtv = 0;
  char buffer[4*MAXLINE];
  char *o1c = &(buffer[MAXLINE]);
  char *exp = &(buffer[3*MAXLINE]);
  char *rval;  // for error checks
  
  tvfile = fopen(tvname, "r");
  if (tvfile == NULL) {
    printf("Test-vector file %s can not be openend!\n", tvname);
    return M25519_ERR_TVFILE;
  }
  printf("Testing gfp_cneg() with test-vector file %s ...\n", tvname);
  
  buffer[4*MAXLINE-1] = '\0';
  rval = fgets(buffer, MAXLINE, tvfile);
  if (rval == NULL) return M25519_ERR_TVFILE;
  buffer[strcspn(buffer, "\r\n")] = '\0';
  rval = strstr(buffer, "Negation");
  if (rval == NULL) printf("Incorrect test-vector file!\n");

  while (rval != NULL) {
    // get next testvector from tv-file
    rval = get_vector(buffer, tvfile);
    if (rval == NULL) break;
    // extract operands from testvector
    mpi_from_hex(op1, &(buffer[1*MAXLINE]), LEN);
    // execute the arithmetic operation
    gfp_cneg(res, op1, (numtv & 1));
    // check result and report mismatch
    wrongtv += chk_vector(o1c, NULL, exp, res);
    numtv++;
  }
  fclose(tvfile);
  
  printf(" -> %i test-vectors verified, ", numtv);
  printf("%i test-vectors wrong\n", wrongtv);
  return numtv;
}
