# Prime-Field Arithmetic

As indicated by its name, Curve25519 is defined over the prime field GF($p$) given by $p = 2^{255} - 19$. This prime is a so-called _pseudo-Mersenne prime_ and allows for very efficient (i.e., linear-time) implementation of the modular reduction. The field-arithmetic operations have a significant impact on both the efficiency and security (i.e., resistance against timing attacks) of X25519 key exchange and Ed25519 signatures. Therefore, Micro25519 comes with its own prime-field arithmetic library, implemented from scratch to achieve a good trade-off between performance and code size.

Similarly to a Multi-Precision Integer (MPI) as described in [mpiarith.md](./mpiarith.md), an element of the prime field GF($p$) is represented as an array of type `Word`, which is defined as an unsigned 32-bit integer (i.e., an `uint32_t`). These arrays have a fixed length of eight words and can, therefore, accommodate up to 256 bits. The word with index 0 is the least-significant one. To give a concrete example, an element $x$ of GF($p$) can be written as $x = [x_7, x_6, x_5, x_4, x_3, x_2, x_1, x_0]$, where $0 \leq x_i < 2^{32}$, and represents the value $x = \sum_{i=0}^7 x_i \cdot 2^{32i}$. 

The prime-field arithmetic covers besides the fundamental operations (e.g., addition, subtraction, multiplication, and inversion) also some special operations like the multiplication of a field-element by a 32-bit constant, the halving of a field-element, the conditional negation of a field-element, etc. Furthermore, some functions for multi-precision integers, such as  `mpi_copy`, `mpi_setw`, and `mpi_print` (see [mpiarith.md](./mpiarith.md)), can be used for field-elements as well since both are represented as `Word`-arrays.

> [!NOTE]
> All arithmetic functions accept incompletely reduced operands as inputs, i.e., a field-element does not necessarily need to be the least non-negative residue modulo $p$. In fact, the functions can handle any input in the range $[0, 2^{256}-1]$ properly. The result of an arithmetic function is also not necessarily fully reduced but is guaranteed to be in the range $[0, 2p-1]$, which means that the least non-negative residue can be obtained by (at most) one subtraction of $p$.

All functions except the inversion are implemented such that they meet the usual requirements of "constant-timeness" in the sense that their execution time depends only on the length of the operands but not on their specific value. More concretely, the functions are written to have an operand-independent execution pattern and do not include any conditional statements (e.g., branches) that could leak sensitive information through small differences in execution time.

The inversion `gfp_inv` is based on the Extended Euclidean Algorithm (EEA) and has an operand-dependent execution pattern and, therefore, an operand-dependent execution time. However, `gfp_inv` can be efficiently and effectively protected against timing attacks by applying a multiplicative masking technique as follows: the field-element $x$ to be inverted is first multiplied by a field-element $u$ that is unknown to the attacker, then the product $x \cdot u$ is inverted, and finally the inverse $(x \cdot u)^{-1}$ is multiplied by $u$ to get $x^{-1}$.


### Initialization of a field-element with p: $r = p$

```
void gfp_setp(Word *r);
```

This function sets a field-element to the prime $p = 2^{255} - 19$.

The word-array `r` for the result (i.e., $p$) must be able to accommodate eight words.


### Comparison of a field-element with p: $a \stackrel{?}{=} p$

```
int gfp_cmpp(const Word *a);
```

This function compares a (potentially not fully-reduced) field-element with the prime $p = 2^{255} - 19$.

The return value is `1` if $a > p$, `0` if $a = p$, or `-1` if $a < p$.


### Addition of two field-elements: $r = a + b \bmod p$

```
void gfp_add(Word *r, const Word *a, const Word *b);
```

This function adds two field-elements modulo $p$, whereby the result (i.e., the modular sum) may not be fully reduced. However, the result is always in the range $[0, 2p-1]$.

The word-array `r` for the result must be able to accommodate eight words.


### Subtraction of a field-element from another field-element: $r = a - b \bmod p$

```
void gfp_sub(Word *r, const Word *a, const Word *b);
```

This function subtracts a field-element from another field-element modulo $p$, whereby the result (i.e., the modular difference) may not be fully reduced. However, the result is always in the range $[0, 2p-1]$.

The word-array `r` for the result must be able to accommodate eight words.


### Conditional negation of a field-element: $r = p - a \bmod p$ or $r = a \bmod p$

```
void gfp_cneg(Word *r, const Word *a, int c);
```

This function negates a field-element modulo $p$ if the condition-value `c` is 1, otherwise the result is simply the field-element. Only the least-significant bit of `c` is considered. Note that, when $c = 0$, the result $r$ is not necessarily identical to $a$, but can also take the value $a + p$ or $a - p$ (if $a > p$). The result (i.e., the field-element or its negative) may not be fully reduced. However, the result is always in the range $[0, 2p-1]$.

The word-array `r` for the result must be able to accommodate eight words.


### Halving of a field-element: $r = a/2 \bmod p$

```
void gfp_hlv(Word *r, const Word *a);
```

This function halves a field-element modulo $p$, whereby the result (i.e., the halve) may not be fully reduced. However, the result is always in the range $[0, 2p-1]$.

The word-array `r` for the result must be able to accommodate eight words.


### Multiplication of two field-elements: $r = a \times b \bmod p$

```
void gfp_mul(Word *r, const Word *a, const Word *b);
```

This function multiplies two field-elements modulo $p$, whereby the result (i.e., the product) may not be fully reduced. However, the result is always in the range $[0, 2p-1]$.

The word-array `r` for the result must be able to accommodate eight words.


### Squaring of a field-element: $r = a^2 \bmod p$


```
void gfp_sqr(Word *r, const Word *a);
```

This function squares a field-element modulo $p$, whereby the result (i.e., the square) may not be fully reduced. However, the result is always in the range $[0, 2p-1]$.

The word-array `r` for the result must be able to accommodate eight words.


### Multiplication of a field-elements by a 32-bit value: $r = a \times b \bmod p$

```
void gfp_mul32(Word *r, const Word *a, const Word *b);
```

This function multiplies a field-element by a 32-bit value modulo $p$, whereby the result (i.e., the product) may not be fully reduced. However, the result is always in the range $[0, 2p-1]$.

The word-array `r` for the result must be able to accommodate eight words.


### Full reduction of a field-element: $r = a \bmod p$

```
void gfp_fred(Word *r, const Word *a);
```

This function computes the least non-negative residue of a field-element modulo $p$. The result (i.e., the residue) is always fully reduced, i.e., in the range $[0, p-1]$.

The word-array `r` for the result must be able to accommodate eight words.


### Comparison of two field-elements: $a \stackrel{?}{=} b$

```
int gfp_cmp(const Word *a, const Word *b);
```

This function compares the least non-negative residue of two field-elements, i.e., the two field-elements are fully reduced before the comparison. 

The return value is `1` if $a > b$, `0` if $a = b$, or `-1` if $a < b$.


### Inversion of a non-0 field-element: $r = a^{-1} \bmod p$

```
int gfp_inv(Word *r, const Word *a);
```

This function computes the multiplicative inverse of a non-0 field-element modulo $p$, whereby the result (i.e., the inverse) may not be fully reduced. However, the result is always in the range $[0, 2p-1]$.

The word-array `r` for the result must be able to accommodate eight words. The return value is `ERR_INVERSION_ZERO` if $a = 0$ and `0` otherwise.
