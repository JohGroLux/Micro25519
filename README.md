# Micro25519

Micro25519 is an elliptic-curve cryptographic library for efficient [X25519 key exchange](https://cr.yp.to/ecdh.html) and [Ed25519 signatures](https://ed25519.cr.yp.to) on 8, 16, and 32-bit microcontrollers. It uses the same C code base across different microcontroller architectures and is written in portable ANSI C99 that can be compiled with various versions of GCC and Clang, as well as the compilers of proprietary software development environments, such as [Keil Microvision](https://www.keil.com) and [IAR Embedded Workbench](https://www.iar.com/embedded-development-tools/iar-embedded-workbench). The library does not require dynamic memory allocation and has no external dependencies except for the random-number generation, which uses the relevant API supported by the operating system or platform SDK of the target device, e.g., `/dev/urandom`. Micro25519's C code base is highly modular, well documented, (relatively) easy to understand, and with only around 3000 LOCs small enough to allow auditing.

Unlike many other elliptic-curve cryptographic libraries, Micro25519 is not solely optimized for speed, but aims for a good trade-off between execution time, RAM footprint, and binary code size. In order to achieve fast execution times, the library comes with highly-optimized Assembly implementations of the most performance-critical arithmetic operations for four target architectures, namely 8-bit [AVR](https://developerhelp.microchip.com/xwiki/bin/view/products/mcu-mpu/8-bit-avr/structure/), 16-bit [MSP430](https://www.ti.com/microcontrollers-mcus-processors/msp430-microcontrollers/overview.html), 32-bit [ARMv7-M](https://developer.arm.com/Processors/Cortex-M3) (e.g., Cortex-M3 microcontrollers) and 32-bit [RISC-V](https://riscv.org/specifications/ratified/) (concretely RV32IMC). Micro25519 has a RAM (i.e., stack) consumption of less than 1 kB and occupies less than 10 kB flash memory for each of these four target architectures when using the Assembly functions. Other microcontroller architectures, which are currently not supported with optimized Assembly functions, can still use the C99 version of Micro25519.

Micro25519 provides a clean and easy-to-use high-level API for X25519 and Ed25519, very similar to that of [Lib25519](https://lib25519.cr.yp.to) and [LibSodium](https://doc.libsodium.org). In addition, the library also comes with a mid-level API for both fixed-base and variable-base scalar multiplication on [Curve2559](https://datatracker.ietf.org/doc/html/rfc7748#section-4.1) and the birationally-equivalent twisted Edwards curve, known as [Edwards25519](https://datatracker.ietf.org/doc/html/rfc8032#section-5). These implementations of scalar multiplication are of independent interest since they can be used for cryptosystems other than X25519 and Ed25519. All high-level functions are thoroughly tested with test vectors from [project Wycheproof](https://github.com/C2SP/wycheproof) and the [CCTV collection](https://github.com/C2SP/CCTV). Furthermore, many low-level functions for arithmetic operations in the underlying prime field, in particular those with Assembly implementations, have their own unit tests.

Micro25519 aims to be fully resistant against timing-based side-channel attacks on the four main target platforms that are supported by Assembly implementations of the field arithmetic. To achieve this, all arithmetic operations in the underlying prime field, except inversion, are written such that they always execute exactly the same sequence of instructions, irrespective of the operands, and therefore have constant execution time. This form of "constant timeness" is also guaranteed by all point-arithmetic operations and the functions for scalar multiplication. The inversion in the prime field is based on the Extended Euclidean Algorithm (EEA), which has operand-dependent execution time, but adopts a simple multiplicative masking technique to thwart timing attacks.

### Tasks and (preliminary) project timeline

- Task 1 (mid March): API specification ✔️
- Task 2 (end April): C implementation of Multi-Precision Integer (MPI) and prime-field arithmetic.
- Task 3 (mid June): RISC-V Assembly implementation of performance-critical MPI/field operations.
- Task 4 (end July): AVR Assembly implementation of performance-critical MPI/field operations.
- Task 5 (mid September): MSP430 Assembly implementation of performance-critical MPI/field operations.
- Task 6 (end October): ARMv7M Assembly implementation of performance-critical MPI/field operations.
- Task 7 (mid December): C implementation of the point arithmetic, scalar multiplication, and high-level functions.

### Funding

This project is funded through [NGI Zero Core](https://nlnet.nl/core), a fund established by [NLnet](https://nlnet.nl) with financial support from the European Commission's [Next Generation Internet](https://ngi.eu) program. Learn more at the [NLnet project page](https://nlnet.nl/project/IotECC).

[<img src="https://nlnet.nl/logo/banner.png" alt="NLnet foundation logo" width="20%" />](https://nlnet.nl)
[<img src="https://nlnet.nl/image/logos/NGI0_tag.svg" alt="NGI Zero Logo" width="20%" />](https://nlnet.nl/core)
