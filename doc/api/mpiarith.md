# Multi-Precision Integer (MPI) Arithmetic

Micro25519 provides a (small) set of functions for basic MPI arithmetic operations (e.g., addition, multiplication) and some auxiliary operations such as setting an MPI to a certain value, comparing two MPIs, converting and MPI from/to a hex-string, etc. For all these functions, an MPI is generally represented as an array of type `Word`, which is defined as an unsigned 32-bit integer (i.e., an `uint32_t`). The word with index 0 is the least-significant one. To give a concrete example, a 256-bit MPI $x$ can be written as $x = [x_7, x_6, x_5, x_4, x_3, x_2, x_1, x_0]$, where $0 \leq x_i < 2^{32}$, and represents the value $x = \sum_{i=0}^7 x_i \cdot 2^{32i}$. 

Most functions offer a high degree of flexibility and do not assume that the operands have a certain fixed length. Instead, the length of the input operands can be passed through a `len` parameter. However, functions that get two or more MPIs as input expect all of them to be the same length (i.e., to consist of the same number of words). Enabling such flexibility for the MPI functions does not create tensions with efficiency since these functions are not performance-critical.

Unless specified otherwise, all functions are implemented such that they meet the usual requirements of "constant-timeness" in the sense that their execution time depends only on the length of the operands, but not their specific value. More concretely, the functions are written to have an operand-independent execution pattern and do not include any conditional statements (e.g., branches) that could leak sensitive information through small differences in execution time.


### Conversion of a hex-string to an MPI

```
void mpi_from_hex(Word *r, const char *hexstr, int len);
```

This function converts a hex-string to an MPI of length `len`. The hex-string pointed to by `hexstr` is expected to start with `0x` and must be null-terminated so that its length can be determined. Therefore, the hex-string should normally consist of $8\cdot len + 2$ characters, but shorter hex-strings will also be converted correctly. If the hex-string is longer, only the $8\cdot len$ rightmost characters are considered.

The word-array `r` for the result (i.e., the obtained MPI) must be able to accommodate `len` words.


### Conversion of an MPI to a hex-string

```
void mpi_to_hex(char *hexstr, const Word *a, int len);
```

This function converts an MPI of length `len` to a hex-string. The hex-string starts with `0x` and is null-terminated.

The char-array `hexstr` for the hex-string must be able to accommodate $8 \cdot len + 3$ characters (two extra characters for the leading `0x` and one extra character for the null-terminator).


### Printing of an MPI as hex-string to `stdout` 

```
void mpi_print(const char *prefix, const Word *a, int len);
```

This function first prints an optional prefix-string to `stdout`, followed by a hex-string representing an MPI of length `len`. The prefix-string can be used to specify, for example, a name for the MPI that is printed or some other information. No prefix is printed when the `prefix` pointer is `NULL` or when the length of the prefix-string is 0. A newline character `\n` is added after the hex-string.

This function only needs a minimalist subset of the features of `printf` from the standard C library since the conversion of the MPI to a hex-string uses `mpi_to_hex` from above. In fact, it suffices when `printf` supports the printing of null-terminated strings (i.e., arrays of type `char`).


### Initialization of an MPI with a 32-bit word: $r = [0,\ldots,0,a]$

```
void mpi_setw(Word *r, Word a, int len);
```

This function sets an MPI of length `len` to a 32-bit word `a`, which means the least-significant word of the MPI is set to `a` and all other words to 0. It can be used, for example, to initialize an MPI with 0, 1, or some other small value.

The word-array `r` for the result (i.e., $[0,\ldots,0,a]$) must be able to accommodate `len` words.


### Comparison of an MPI with a 32-bit word: $a \stackrel{?}{=} [0,\ldots,0,b]$

```
int mpi_cmpw(const Word *a, Word b, int len);
```

This function compares an MPI of length `len` with a 32-bit value `b`, which corresponds to an MPI of the form $[0, \ldots, 0, 0, b]$. It can be used, for example, to check whether an MPI is 0 or 1.

The return value is `1` if $a > [0, \ldots, 0, 0, b]$, `0` if $a = [0, \ldots, 0, 0, b]$, or `-1` if $a < [0, \ldots, 0, 0, b]$.


### Comparison of two MPIs: $a \stackrel{?}{=} b$

```
int mpi_cmp(const Word *a, const Word *b, int len);
```

This function compares two MPIs of length `len`. 

The return value is `1` if $a > b$, `0` if $a = b$, or `-1` if $a < b$.


### Copying an MPI: $r = a$

```
void mpi_copy(Word *r, const Word *a, int len);
```

This function copies an MPI of length `len` from source `a` to destination `r`.

The word-array `r` for the result (i.e., the destination) must be able to accommodate `len` words.


### 1-bit right-shift of an MPI: $r = a \gg 1$

```
int mpi_shr(Word *r, const Word *a, int len);
```

This function shifts an MPI of length `len` one bit to the right, which corresponds to a division by 2.

The word-array `r` for the result must be able to accommodate `len` words. The return value is either 0 or 1 and corresponds to the least-significant bit of `a` before the shift operation.


### Addition of two MPIs: $r = a + b$

```
int mpi_add(Word *r, const Word *a, const Word *b, int len);
```

This function adds two MPIs of length `len`. It can also be used to shift an MPI one bit to the left, which corresponds to a multiplication by 2.

The word-array `r` for the result must be able to accommodate `len` words. The return value is either 0 or 1, depending on whether the addition produced a carry.


### Conditional addition of an MPI to another MPI: $r = a + b$ or $r = a$

```
int mpi_cadd(Word *r, const Word *a, const Word *b, int c, int len);
```

This function adds the second MPI to the first MPI, both of which have length `len`, if the condition-value `c` is 1, otherwise the result is simply the first MPI. Only the least-significant bit of `c` is considered. Consequently, this function computes $r = a + c \cdot b$, where $c$ is either 0 or 1.

The word-array `r` for the result must be able to accommodate `len` words. The return value is either 0 or 1, depending on whether an addition was actually performed and produced a carry.


### Subtraction of an MPI from another MPI: $r = a - b$

```
int mpi_sub(Word *r, const Word *a, const Word *b, int len);
```

This function subtracts the second MPI from the first MPI, both of which have length `len`.

The word-array `r` for the result must be able to accommodate `len` words. The return value is either 0 or 1, depending on whether the subtraction produced a borrow.


### Multiplication of two MPIs: $r = a \times b$

```
void mpi_mul(Word *r, const Word *a, const Word *b, int len);
```

This function multiplies two MPIs of length `len`, yielding a product of $2 \cdot len$ words.

The word-array `r` for the result must be able to accommodate $2 \cdot len$ words.

