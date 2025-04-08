# Arithmetic on Twisted Edwards (TED) Curve

The TED model of an elliptic curve was discovered in 2008 and has since established itself as an attractive alternative to the conventional Weierstrass model. A TED curve is specified by an equation of the form $a x^2 + y^2 = 1 + d x^2 y^2$ over a finite field GF($q$) of odd characteristic, which in the case of the Edwards25519 curve is the prime field GF($p$) with $p = 2^{255} - 19$. A unique feature of these curves is a _complete addition law_ and the existence of a neutral element for point addition, namely the affine point $O = (0,1)$. The "complete" refers to the fact that the addition law yields the correct sum $P + Q$ for any pair of points $P$, $Q$ that satisfy the curve equation, including corner cases like $P = O$, $Q = O$, $P = -Q$, $Q = -P$, and so on. This addition law is particularly efficient when the curve parameter $a$ is fixed to $-1$. All algorithms for scalar multiplication on Weierstrass curves, ranging from the basic "double-and-add" method to advanced techniques, like window and comb methods, are directly applicable to TED curves.

TED curves can outperform not only ordinary Weierstrass curves but also Montgomery curves for variable-base scalar multiplication. However, the high performance of TED curves for variable-base scalar multiplication comes at the expense of enormous RAM consumption, whereas the Montgomery ladder on Montgomery curves has an extremely small RAM footprint. Therefore, it makes sense to use the TED model primarily for fixed-base scalar multiplication $Q = l \cdot G$, such as is needed for the generation of an EdDSA signature. Due to the birational equivalence between the TED model and the Montgomery model, a variable-base scalar multiplication $Q = k \cdot P$ can be performed on Curve25519, which is birationally-equivalent to Edwards25519. This requires a TED-Montgomery mapping of $P$ before and a Montgomery-TED mapping of $Q$ after the scalar multiplication. These mappings are relatively simple, though certain [corner cases](https://orbilu.uni.lu/handle/10993/49970) have to be taken into account. However, dealing with these corners cases is fairly straightforward when insisting that $P$ does not have low order.

Micro25519 supports both fixed-base and variable-base scalar multiplication on Edwards25519, and includes all necessary functions for point arithmetic (i.e., point addition and doubling) as well as various auxiliary functions (e.g., to map points from the TED curve to the Montgomery curve and vice versa, to check whether a point actually lies on the curve, to check whether a point has low order, etc.). The fixed-base scalar multiplication $R = l \cdot G$ is performed directly on Edwards25519, whereas the variable-base scalar multiplication $R = k \cdot P$ uses the birationally-equivalent Curve25519 with point-mappings in both directions.

The point addition/doubling on Edwards25519 takes advantage of a variant of the so-called _extended projective coordinates_, which were originally defined as coordinates of the form $[X:Y:Z:T]$ where $T = X \cdot Y / Z$. Micro25519 uses an extension of this coordinate system where a point is represented as $[X:Y:Z:E:H]$ with $E \cdot H = T = X \cdot Y / Z$, i.e., the fourth coordinate $T$ is further divided into $E$ and $H$. When performing a fixed-base scalar multiplication according to the _comb method_, the point additions are actually _mixed additions_, which means that one point is given in extended projective coordinates $[X:Y:Z:E:H]$ and the other point in extended affine coordinates of the form $(u,v,w) = ((x+y)/2, (y-x)/2, d \cdot x \cdot y)$.


### Data structures

The coordinates of a point on an elliptic curve are elements of the underlying prime field and stored in arrays of type `Word`, which are nothing else than `uint32_t`-arrays (see [gfparith.md](./gfparith.md)). Micro25519 uses two special data structures to simplify point arithmetic: one to represent points and the other to combine all domain parameters of the two curve models, e.g., the parameter $A$ of Curve25519, the parameter $d$ of Edwards25519, the cardinality of the elliptic-curve group, and so on.

```
typedef struct point {
  int dim;    // dimension: 1 <= dim <= 6
  Word *xyz;  // pointer to coordinates-array
} Point;
```

`Point` is a simple but highly flexible C structure to represent a point on an elliptic curve in affine, projective, or extended projective coordinates. Other ECC libraries use different data structures for affine and projective points, commonly a `struct` with two or three fixed-length word-arrays for the coordinates, but a `struct` composed of an integer specifying the number of coordinates (i.e., the dimension of the point) and a pointer to a single word-array containing all coordinates offers more flexibility. Typical use cases for the different dimensions are as follows:

- dim = 2: conventional affine $(x,y)$ coordinate system
- dim = 3: conventional projective $[X:Y:Z]$ or extended affine $(u,v,w)$ coordinate system
- dim = 4: same as dim = 3, but with further space for an intermediate result
- dim = 5: extended projective $[X:Y:Z:E:H]$ coordinate system
- dim = 6: same as dim = 5, but with further space for an intermediate result

The C structure `ECDomPar` contains information about the underlying prime field, the curve parameters (for the Montgomery and TED form, respectively) and various curve-dependent constants needed for X25519 or Ed25519, all accessible from one place. In this way, the implementation can easily be adapted to support other curves. Micro25519 places the arrays in which these parameters and constants are stored in non-volatile memory (i.e., flash) to reduce the RAM footprint.

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

The `tbl` element of `ECDomPar` is a pointer to a pre-computed table containing eight multiples of the generator $G$ of a prime-order subgroup of Edwards25519, which is used to speed up the fixed-base scalar multiplication. Each point in this table is represented by extended affine coordinates and occupies 24 words or 96 bytes in flash memory.


### Initialization of a point with $O$: $R = O$

```
void ted_set0(Point *r);
```

This function initializes a point with the neutral element $O$, which is $(0,1)$ in affine coordinates (i.e., `r->dim` is 2), $[0:1:1]$ in projective coordinates (i.e., `r->dim` is 3 or 4), and $[0:1:1:0:1]$ in extended projective coordinates (i.e, `r->dim` is 5 or 6).


### Printing a point to `stdout`

```
void ted_print(const Point *p);
```

This function prints the coordinates of a point as hex-strings to `stdout`. Depending on the dimension of `p`, either two or three coordinates are printed.


### Copying a point: $R = P$

```
void ted_copy(Point *r, const Point *p);
```

This function copies a point from source `p` to destination `r`, taking the dimension of both into account. More concretely, the function can handle not only the case when the source and destination have the same dimension, but also when their dimension differs, provided that `r->dim` is not 2. When `r->dim` is 2 and `p->dim` is bigger than 2 (which corresponds to a conversion from projective or extended projective to affine coordinates), the function `ted_conv_p2a` must be used. This function can also not be used for copying a point from the pre-computed table of multiples of the generator $G$ since these points are represented by simple `Word`-arrays and not `Point` structures. Instead, to access a point from the table, the function `ted_load_point` must be used.


### Loading a point from pre-computed table: $R = \mathrm{Tbl}[i]$

```
void ted_load_point(Point *r, const Word *tbl, int idx);
```

This function loads a point in extended affine coordinates of the form $(u,v,w) = ((x+y)/2, (y-x)/2, d \cdot x \cdot y)$ from a pre-computed table of eight multiples of the generator $G$. The pre-computed table is actually a linear `Word`-array containing 24 coordinates (i.e., 72 words) and not an array of `Point` structures. The three least-significant bits of `idx` determine the index of the table-entry that is loaded and the fourth least-significant bit determines whether the loaded point gets negated. Only these four bits of `idx` are considered.

Note that `r->dim` must be (at least) 3, but the result `r` is given in extended affine coordinates and not projective coordinates. Therefore, `r` can not be used as a source or destination of the `ted_copy` function.


### Conversion from extended affine to extended projective coordinates: $R = [X:Y:Z:E:H]$

```
void ted_conv_ea2ep(Point *r, const Point *p);
```

This function converts a point in extended affine $(u,v,w)$ coordinates, such as obtained by `ted_load_point`, to a point in extended projective $[X:Y:Z:E:H]$ coordinates.


### Mixed point addition: $R = R + P$

```
void ted_add(Point *r, const Point *p);
```

This function adds a point in extended affine $(u,v,w)$ coordinates, such as obtained by `ted_load_point`, to a point in extended projective $[X:Y:Z:E:H]$ coordinates. 

Note that `r->dim` must be 6 since the function uses the sixth coordinate of `r` to store an intermediate result of the point addition (the first five coordinates contain $X$, $Y$, $Z$, $E$, and $H$, respectively). The point `p` must have a dimension of (at least) 3.


### Extended projective point doubling: $R = 2 \cdot R$

```
void ted_double(Point *r)
```

This function doubles a point in extended projective $[X:Y:Z:E:H]$ coordinates. 

Note that `r->dim` must be 6 since the function uses the sixth coordinate of `r` to store an intermediate result of the point doubling (the first five coordinates contain $X$, $Y$, $Z$, $E$, and $H$, respectively).


### Checking whether a point has low order: $\mathrm{ord}(P) \stackrel{?}{\geq} 8$

```
int ted_check_order(Point *r, const Point *p);
```

This function checks whether a point $P$ given in affine coordinates has low order by computing $R = 8 \cdot P$ through three point doublings and determining whether the $Z$-coordinate of $R$ is 0. Such a check can help prevent certain kinds of side-channel attack that specifically target points of low order.

The return value is `0` if $P$ does not have low order, and `ERR_INVALID_POINT` otherwise (i.e., $P$ has an order of 2, 4, or 8).


### Checking whether a point satisfies the curve equation: $-x^2 + y^2 \stackrel{?}{=} d x^2 y^2$

```
int ted_valid_point(const Point *p);
```

This function checks whether a point $P$ given in affine coordinates satisfies the curve equation with $a = -1$. Such a check can help prevent certain kinds of attack enabled by invalid points.

The return value is `0` if $P$ satisfies the curve equation, and `ERR_INVALID_POINT` otherwise.


### Computing the "raw" fixed-base comb method: $R = l  \cdot g$

```
void ted_mul_comb4b(Point *r, const Word *l, const ECDomPar *d);
```

This function performs the fixed-base comb method on the generator $G$, processing four bits of the scalar at a time. The scalar $l$ is not validated, so this function should be used with care. After completion of the comb method, the result $R$ is given in extended projective coordinates of the form $[X:Z:Y:E:H]$. The array `l` containing the scalar must have a length of eight words. This comb implementation has an operand-independent execution profile and can resist timing attacks against `l` on microcontrollers without cache. The parameter `d` is needed to access the pre-computed table containing eight multiples of $G$, which is used by the comb method.

Note that `r->dim` must be 6 since the function uses the sixth coordinate to store an intermediate result of the point addition (the first five coordinates contain $X$, $Y$, $Z$, $E$, and $H$, respectively).


### Conversion from projective to affine coordinates: $R = (x,y)$

```
int ted_conv_p2a(Point *r, const Point *p, const ECDomPar *d);
```

This function converts a point given in projective coordinates to a point in affine coordinates. Such a conversion requires inversion of the $Z$-coordinate, which can leak information about the secret scalar used to compute $P$ when implemented in a straightforward way according to the Extended Euclidean Algorithm (EEA). To prevent this leakage, a simple masking technique is applied, which means that instead of inverting $Z$ "directly", it is first multiplied by a field element $U$ that is unknown to the attacker and then the product $Z \cdot U$ is inverted. Finally, the inverse $(Z \cdot U)^{-1}$ is multiplied by $U$ to obtain $Z^{-1}$.

The return value is `0` if $R$ is a valid point and `ERR_INVALID_POINT` if the $Z$-coordinate of $R$ is 0.


### Fixed-base scalar multiplication: $R = l \cdot G$

```
int ted_mul_fixbase(Point *r, const Word *l, const ECDomPar *d);
```

This function computes a fixed-base scalar multiplication $R = l \cdot G$, including a validation of inputs and the result. The base point $G$ is the generator of a prime-order subgroup. In the case of Edwards25519, the generator has the $y$-coordinate $(4/5) \in \mathrm{GF}(p)$. The array `l` containing the scalar must have a length of eight words. This implementation of fixed-base scalar multiplication is supposed to resist timing attacks against `l` on microcontrollers without cache. The parameter `d` is needed to access the pre-computed table containing eight multiples of $G$ for the underlying comb method.

The result $R$ is represented in affine coordinates. The return value is `0` when the scalar and the result are valid and non-0 otherwise. Possible non-0 return values are `ERR_INVALID_SCALAR` (when the scalar $l = 0$) and `ERR_INVALID_POINT` (when $R$ is the neutral element).


### Variable-base scalar multiplication: $R = k \cdot P$

```
int ted_mul_varbase(Point *r, const Word *k, const Point *p, const ECDomPar *d);
```

This function computes a variable-base scalar multiplication $R = k \cdot P$, including a validation of inputs and the result. The base point $P$ must be given in affine coordinates and must not have a low order (i.e., $\mathrm{ord}(P) > 8$). The array `k` containing the scalar must have a length of eight words. Note that the computation of $R = k \cdot P$ is actually performed on the birationally-equivalent Curve25519 with a mapping of the base point $P$ to Curve25519 before and a mapping of the result $R$ to Edwards25519 after the scalar multiplication. The parameter `d` is needed to access a pre-computed constant for the mapping of points from the TED model to the Montgomery model and vice versa.

The result $R$ is represented in affine coordinates. The return value is `0` when all inputs and the result are valid and non-0 otherwise. Possible non-0 return values are `ERR_INVALID_SCALAR` (when the scalar $k = 0$) and `ERR_INVALID_POINT` (when $P$ has low order or $R$ is the neutral element).


### Mapping of point on TED curve to Montgomery curve: $R_{MON} = P_{TED}$

```
void mon_to_ted(Point *r, const Point *p, const ECDomPar *d);
```

This function maps a projective point on a TED curve to the corresponding point on the birationally-equivalent Montgomery curve. The parameter `d` is needed to access a pre-computed constant required for the mapping.

The result $R$ is given in conventional projective $[X:Y:Z]$ coordinates. 
