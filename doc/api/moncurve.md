# Arithmetic on Montgomery Curve

Curve25519, in its original form, was defined as a Montgomery curve with the goal of enabling fast ECDH key exchange. A Montgomery curve is specified by an equation of the form $B y^2 = x^3 + A x^2 + x$ over a finite field GF($q$) of odd characteristic, which in the case of Curve25519 is the prime field GF($p$) with $p = 2^{255} - 19$. A unique feature of these curves is a very efficient _differential addition law_ that involves only the (projective) $X$ and $Z$ coordinates of the points. The "differential" refers to the fact that the addition formula yields the sum $S = P_1 + P_2$ of two points, $P_1$ and $P_2$, whose difference $D = P_1 - P_2$ is known. This "x-ccordinate-only" addition law is the basis for the so-called _Montgomery ladder_ for variable-base scalar multiplication $Q = k \cdot P$, which can be used, for example, to derive a shared secret in ECDH key exchange. The $y$-coordinate of $Q$, if needed, can be recovered at the end of the ladder, provided that the $y$-coordinate of $P$ is known.

The Montgomery addition law is not particularly efficient when the base point $P$ is fixed and known in advance, which is the case for the generation of a key-pair for ECDH key exchange. However, fixed-base scalar multiplication is the domain in which the Twisted Edwards (TED) model excels. Due to the birational equivalence between the Montgomery model and the TED model, the individual arithmetic strengths of these two curve models can be combined. For example, to generate an ECDH key-pair, the fixed-base scalar multiplication $Q = l \cdot G$ can be performed on the Edwards25519 curve (which is birationally-equivalent to Curve25519), and the obtained result $Q$ mapped to the corresponding point on Curve25519.

Micro25519 supports both fixed-base and variable-base scalar multiplication on Curve25519, and includes all necessary functions for point arithmetic (i.e., differential point addition and doubling) as well as various auxiliary functions (e.g., to recover the $Y$-coordinate at the end of the Montgomery ladder, to map points from the Montgomery curve to the TED curve and vice versa, to check whether a point has low order, etc.). The variable-base scalar multiplication $R = k \cdot P$ is performed directly on Curve25519, whereas the fixed-base scalar multiplication $R = l \cdot G$ uses the birationally-equivalent Edwards25519 curve and maps the result to Curve25519.


### Data structures

The coordinates of a point on an elliptic curve are elements of the underlying prime field and stored in arrays of type `Word`, which are nothing else than `uint32_t`-arrays (see [gfparith.md](./gfparith.md)). Micro25519 uses two special data structures to simplify point arithmetic: one to represent points and the other to combine all domain parameters of the two curve models, e.g., the parameter $A$ of Curve25519, the parameter $d$ of Edwards25519, the cardinality of the elliptic-curve group, and so on.

```
typedef struct point {
  int dim;    // dimension: 1 <= dim <= 6
  Word *xyz;  // pointer to coordinates-array
} Point;
```

`Point` is a simple but highly flexible C structure to represent a point on an elliptic curve in affine, projective, or extended projective coordinates. Other ECC libraries use different data structures for affine and projective points, commonly a `struct` with two or three fixed-length word-arrays for the coordinates, but a `struct` composed of an integer specifying the number of coordinates (i.e., the dimension of the point) and a pointer to a single word-array containing all coordinates offers more flexibility. Typical use cases for the different dimensions are as follows:

- dim = 1: x-coordinate only (e.g., X25519 key exchange)
- dim = 2: conventional affine $(x,y)$ or projective $[X:Z]$ coordinate system
- dim = 3: conventional projective $[X:Y:Z]$ coordinate system
- dim = 4: same as dim = 3, but with further space for an intermediate result

The C structure `ECDomPar` contains information about the underlying prime field, the curve parameters (for the Montgomery and TED form, respectively) and various curve-dependent constants needed for X25519 or Ed25519, all accessible from one place. In this way, the implementation can be easily adapted to support other curves. Micro25519 places the arrays in which these parameters and constants are stored in non-volatile memory (i.e., flash) to reduce the RAM footprint.

```
typedef struct ecdompar {  // elliptic curve domain parameters
  const int k;      // bitlength of the prime ($k = 255$)
  const Word c;     // constant $c = 19$ defining the prime $p = 2^k - c$
  const Word *a24;  // constant $(A+2)/4 = 121666$ of the Montgomery curve
  const Word *dte;  // parameter $d$ of corresponding TED curve with $a = -1$
  const Word *rma;  // root of $-a = -(A+2)/B$ for point-mappings MON <-> TED
  const Word *rm1;  // root of $-1$ ($2^{(p-1)/4} \bmod p$) for decompression
  const Word *car;  // cardinality of elliptic-curve group ($8 \cdot \ell$)
  const Word *cbr;  // constant for Barrett reduction modulo the cardinality
  const Word *tbl;  // table of pre-computed points for fixed-base comb method
} ECDomPar;
```

The `tbl` element of `ECDomPar` is a pointer to a pre-computed table containing eight multiples of the generator of a prime-order subgroup of Edwards25519, which is used to speed up the fixed-base scalar multiplication.


### Initialization of a point with $O$: $R = O$

```
void mon_set0(Point *r);
```

This function initializes a point with the "point at infinity," which has the explicit form $[0:1:0]$ in projective coordinates. The point at infinity does not exist in affine coordinates, but it is common practice to use $(0,1)$ as a replacement since these coordinates never satisfy the curve equation. Depending on the dimension of `r`, the result is $x = 0$ (when `r->dim` is 1), or $(x,y) = (0,1)$ (when `r->dim` is 2), or $[X:Y:Z] = [0:1:0]$ (when `r->dim` is 2). When $R$ is the target point of an x-coordinate-only Montgomery ladder, it is initialized to $[X:Z] = [1:0]$.


### Printing a point to `stdout`

```
void mon_print(const Point *p);
```

This function prints the coordinates of a point as hex-strings to `stdout`. Depending on the dimension of `p`, either one, two, or three coordinates are printed.


### Copying a point: $R = P$

```
void mon_copy(Point *r, const Point *p);
```

This function copies a point from source `p` to destination `r`, taking the dimension of both into account. More concretely, the function can handle the cases when the source and destination have both dimension 2 or dimension 3, or when `r->dim` is 3 and `p->dim` is 2. However, when `r->dim` is 2 and `p->dim` is 3 (which corresponds to a conversion from projective to affine coordinates), the function `mon_conv_p2a` must be used.


### Differential point addition: $R = R + P$

```
void mon_add(Point *r, const Point *p, const Word *xd);
```

This function adds a point to another point using projective $[X:Z]$ coordinates (i.e., the $Y$-coordinate is not used). The parameter `xd` is the (affine) $x$-coordinate of the difference $D = P - Q$, which is normally the same as the $x$-coordinate of the base point when performing a scalar multiplication with the Montgomery ladder.

Note that `r->dim` must be 4 since the function uses two coordinates of `r`, namely the second and fourth, to store intermediate results of the point addition (the first and third coordinate contain $X$ and $Z$, respectively). The point `p` can have dimension 2 with $X$ and $Z$ stored in the first and second coordinate, respectively.


### Differential point doubling: $R = 2 \cdot R$

```
void mon_double(Point *r, const ECDomPar *d)
```

This function doubles a point using projective $[X:Z]$ coordinates (i.e., the $Y$-coordinate is not used). The parameter `d` is needed to access the pre-computed constant $a_{24} = (A+2)/4$.

Note that `r->dim` must be 4 since the function uses two coordinates of `r`, namely the second and fourth, to store intermediate results of the point addition (the first and third coordinate contain $X$ and $Z$, respectively).


### Checking whether a point has low order: $\mathrm{ord}(P) \stackrel{?}{\geq} 8$

```
int mon_check_order(Point *r, const Point *p);
```

This function checks whether a point $P$ has low order by computing $R = 8 \cdot P$ through three point doublings and determining whether the $Z$-coordinate of $R$ is 0. Only the (affine) $x$-coordinate of the base point $P$ is needed. Such a check can help prevent certain kinds of side-channel attack that specifically target points of low order.

The return value is `0` if $P$ does not have low order, and `ERR_INVALID_POINT` otherwise (i.e., $P$ has an order of 2, 4, or 8).


### Computing the "raw" Montgomery ladder: $R = k  \cdot P$

```
void mon_mul_ladder(Point *r, const Word *k, const Word *xp, const ECDomPar *d);
```

This function performs the Montgomery ladder on the (affine) $x$-coordinate $x_p$ of the base point $P$. Neither $x_p$ nor the scalar $k$ is validated, so this function should be used with care. After completion of the ladder, the result $R$ is given in projective coordinates of the form $[X:Z]$, i.e., the $Y$-coordinate is not computed. Besides the $X$ and $Z$-coordinate of $R = k \cdot P$, the $X$ and $Z$-coordinate of the point $S = R + P = (k+1) \cdot P$ are also computed by the Montgomery ladder. The array `k` containing the scalar must have a length of eight words. This ladder implementation has an operand-independent execution profile and can resist timing attacks against `k` on microcontrollers without cache. The parameter `d` is needed to access the pre-computed constant $a_{24} = (A+2)/4$.

Note that `r->dim` must be 4 since the function uses two coordinates of `r`, namely the second and fourth, to store intermediate results of the point addition (the first and third coordinate contain $X$ and $Z$, respectively). At the end of the ladder, the $X$ and $Z$-coordinate of the point $S = R + P$ are copied to the second and fourth coordinate, respectively. These four coordinates, along with the affine $x$ and $y$-coordinate of the base point $P$, can be used to recover the $Y$-coordinate of $R$.


### Recovery of $Y$-coordinate from Montgomery ladder: $R = [X:Y:Z]$

```
void mon_recover_y(Point *r, const Point *p, const ECDomPar *d);
```

This function recovers the projective $Y$-coordinate from the projective $X$ and $Z$-coordinate of the points $R = k \cdot P$ $X$ and $S = R + P = (k+1) \cdot P$, respectively, which are obtained by the Montgomery ladder. The $X$ and $Z$-coordinate of $R$ are contained in the first and third coordinate of `r` and the $X$ and $Z$-coordinate of $S$ in the second and fourth coordinate. Besides these four projective coordinates, the affine $x$ and $y$-coordinate of the base point $P$ are also needed as input.

The result $R$ is given in conventional projective $[X:Y:Z]$ coordinates. Note that recovery of the $Y$-coordinate normally changes the value of the $X$ and $Z$ coordinate, but the fraction $X/Z$ remains the same.


### Conversion from projective to affine coordinates: $R = (x,y)$

```
int mon_conv_p2a(Point *r, const Point *p, const ECDomPar *d);
```

This function converts a point given in projective coordinates to a point in affine coordinates. Depending on the dimension of `r`, either only the affine $x$-coordinate (when `r->dim` is 1) or both the $x$ and $y$-coordinate (when `r->dim` is at least 2) are computed. Such a conversion requires inversion of the $Z$-coordinate, which can leak information about the secret scalar used to compute $P$ when implemented in a straightforward way according to the Extended Euclidean Algorithm (EEA). To prevent this leakage, a simple masking technique is applied, which means that instead of inverting $Z$ "directly", it is first multiplied by a field element $U$ that is unknown to the attacker and then the product $Z \cdot U$ is inverted. Finally, the inverse $(Z \cdot U)^{-1}$ is multiplied by $U$ to obtain $Z^{-1}$.

The return value `0` if $R$ is a valid point and `ERR_INVALID_POINT` if the $Z$-coordinate of $R$ is 0.


### Variable-base scalar multiplication: $R = k \cdot P$

```
int mon_mul_varbase(Point *r, const Word *k, const Point *p, const ECDomPar *d);
```

This function computes a variable-base scalar multiplication $R = k \cdot P$, including a validation of inputs and the result. The base point $P$ must be given in affine coordinates and must not have a low order (i.e., $\mathrm{ord}(P) > 8$). It can consist of either the $x$-coordinate alone (`p->dim` is 1) or of both the $x$ and $y$-coordinate (`p->dim` is 2). The array `k` containing the scalar must have a length of eight words. This implementation of variable-base scalar multiplication is supposed to resist timing attacks against `k` on microcontrollers without cache. The parameter `d` is needed to access the pre-computed constant $a_{24} = (A+2)/4$.

The result $R$ is represented in affine coordinates and consists of either the $x$-coordinate alone (when either `r->dim` or `p->dim` is 1) or of both the $x$ and $y$-coordinate (when `r->dim` and `r->dim` is 2). The return value is `0` when all inputs and the result are valid and non-0 otherwise. Possible non-0 return values are `ERR_INVALID_SCALAR` (when the scalar $k = 0$) and `ERR_INVALID_POINT` (when $P$ has low order or $R$ is the point at infinity).


### Fixed-base scalar multiplication: $R = l \cdot G$

```
int mon_mul_fixbase(Point *r, const Word *l, const ECDomPar *d);
```

This function computes a fixed-base scalar multiplication $R = l \cdot G$, including a validation of inputs and the result. The base point $G$ is the generator of a prime-order subgroup. In the case of Curve25519, the generator has a small $x$-coordinate, namely $x_G = 9$. The array `l` containing the scalar must have a length of eight words. Note that the computation of $R = l \cdot G$ is actually performed on the birationally-equivalent TED curve Edwards25519 and the result is mapped to Curve25519. The parameter `d` is needed to access a pre-computed constant for the mapping of points from the TED model to the Montgomery model.

The result $R$ is represented in affine coordinates and consists of the $x$-coordinate alone (when `r->dim` is 1) or of both the $x$ and $y$-coordinate (when `r->dim` is 2). The return value is `0` when the scalar and the result are valid and non-0 otherwise. Possible non-0 return values are `ERR_INVALID_SCALAR` (when the scalar $l = 0$) and `ERR_INVALID_POINT` (when $R$ is the point at infinity).


### Mapping of point on Montgomery curve to TED curve: $R_{TED} = P_{MON}$

```
void mon_to_ted(Point *r, const Point *p, const ECDomPar *d);
```

This function maps a projective point on a Montgomery curve to the corresponding point on the birationally-equivalent TED curve. The parameter `d` is needed to access a pre-computed constant required for the mapping.

The result $R$ is given in conventional projective $[X:Y:Z]$ coordinates. 
