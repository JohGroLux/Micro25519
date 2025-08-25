###############################################################################
##### Generation of Corner-Case Test-Vectors (CCTV) for GF(p) Arithmetic ######
###############################################################################

# Define the prime p
p = 2**255 - 19

# Define corner-case operands
operands = [
    0,                                  # 0
    1,                                  # 1
    2**256 - 1,                         # -1
    19,                                 # c
    2**256 - 19,                        # -c
    18,                                 # c-1
    2**256 - 18,                        # -(c-1)
    20,                                 # c+1
    2**256 - 20,                        # -(c+1)
    38,                                 # 2c
    2**256 - 38,                        # -2c
    37,                                 # 2c-1
    2**256 - 37,                        # -(2c-1)
    39,                                 # 2c+1
    2**256 - 39,                        # -(2c+1)
    p,                                  # p
    2**256 - p,                         # -p
    p - 1,                              # p-1     
    2**256 - (p - 1),                   # -(p-1)
    p + 1,                              # p+1
    2**256 - (p + 1),                   # -(p+1)
    2*p,                                # 2p
    2**256 - 2*p,                       # -2p
    2*p - 1,                            # 2p-1
    2**256 - (2*p - 1),                 # -(2p-1)
    2*p + 1,                            # 2p+1
    2**256 - (2*p + 1),                 # -(2p+1)
    2**32 - 1,                          # a[0] = 0xFFFFFFFF, rest 0
    2**256 - 2**32,                     # a[0] = 0, rest 0xFFFFFFFF
    2**224 - 1,                         # a[7] = 0, rest 0xFFFFFFFF
    2**256 - 2**224,                    # a[7] = 0xFFFFFFFF, rest 0
    2**224 - 2**32,                     # a[0] = a[7] = 0, rest 0xFFFFFFFF
    2**256 - (2**224 - 2**32) - 1,      # a[0] = a[7] = 0xFFFFFFFF, rest 0
    0x0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF,
    0x89ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF01234567,
]


def gentv_gfp_add(tvfilename):
    numtv = 0
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Corner-Case Test-Vectors (CCTV) for Modular Addition\n")
        for idx1, op1 in enumerate(operands):
            for idx2, op2 in enumerate(operands):
                res = (op1 + op2) % p
                tvfile.write(f"op1: 0x{op1:064X}\n")
                tvfile.write(f"op2: 0x{op2:064X}\n")
                tvfile.write(f"res: 0x{res:064X}\n")
                numtv += 1
        tvfile.close()
    print(f"{numtv} corner-case test-vectors written to {tvfilename}")


def gentv_gfp_sub(tvfilename):
    numtv = 0
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Corner-Case Test-Vectors (CCTV) for Modular Subtraction\n")
        for idx1, op1 in enumerate(operands):
            for idx2, op2 in enumerate(operands):
                res = (op1 - op2) % p
                tvfile.write(f"op1: 0x{op1:064X}\n")
                tvfile.write(f"op2: 0x{op2:064X}\n")
                tvfile.write(f"res: 0x{res:064X}\n")
                numtv += 1
        tvfile.close()
    print(f"{numtv} corner-case test-vectors written to {tvfilename}")


def gentv_gfp_mul(tvfilename):
    numtv = 0
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Corner-Case Test-Vectors (CCTV) for Modular Multiplication\n")
        for idx1, op1 in enumerate(operands):
            for idx2, op2 in enumerate(operands):
                res = (op1 * op2) % p
                tvfile.write(f"op1: 0x{op1:064X}\n")
                tvfile.write(f"op2: 0x{op2:064X}\n")
                tvfile.write(f"res: 0x{res:064X}\n")
                numtv += 1
        tvfile.close()
    print(f"{numtv} corner-case test-vectors written to {tvfilename}")


def gentv_gfp_mul32(tvfilename):
    numtv = 0
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Corner-Case Test-Vectors (CCTV) for Modular Multiplication (32 bit)\n")
        for idx1, op1 in enumerate(operands):
            res = (op1 * 121666) % p
            tvfile.write(f"op1: 0x{op1:064X}\n")
            tvfile.write(f"res: 0x{res:064X}\n")
            numtv += 1
        tvfile.close()
    print(f"{numtv} corner-case test-vectors written to {tvfilename}")


def gentv_gfp_sqr(tvfilename):
    numtv = 0
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Corner-Case Test-Vectors (CCTV) for Modular Squaring\n")
        for idx1, op1 in enumerate(operands):
            res = (op1 * op1) % p
            tvfile.write(f"op1: 0x{op1:064X}\n")
            tvfile.write(f"res: 0x{res:064X}\n")
            numtv += 1
        tvfile.close()
    print(f"{numtv} corner-case test-vectors written to {tvfilename}")


def gentv_gfp_hlv(tvfilename):
    numtv = 0
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Corner-Case Test-Vectors (CCTV) for Modular Halving\n")
        for idx1, op1 in enumerate(operands):
            res = (op1 * pow(2, -1, p)) % p
            tvfile.write(f"op1: 0x{op1:064X}\n")
            tvfile.write(f"res: 0x{res:064X}\n")
            numtv += 1
        tvfile.close()
    print(f"{numtv} corner-case test-vectors written to {tvfilename}")


def gentv_gfp_cneg(tvfilename):
    numtv = 0
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Corner-Case Test-Vectors (CCTV) for Conditional Modular Negation\n")
        for idx1, op1 in enumerate(operands):
            res = op1 % p
            tvfile.write(f"op1: 0x{op1:064X}\n")
            tvfile.write(f"res: 0x{res:064X}\n")
            res = (p - (op1 % p)) % p
            tvfile.write(f"op1: 0x{op1:064X}\n")
            tvfile.write(f"res: 0x{res:064X}\n")
            numtv += 2
        tvfile.close()
    print(f"{numtv} corner-case test-vectors written to {tvfilename}")


if __name__ == "__main__":
    gentv_gfp_add("gfp_add_cc.tv")
    gentv_gfp_sub("gfp_sub_cc.tv")
    gentv_gfp_mul("gfp_mul_cc.tv")
    gentv_gfp_mul32("gfp_mul32_cc.tv")
    gentv_gfp_sqr("gfp_sqr_cc.tv")
    gentv_gfp_hlv("gfp_hlv_cc.tv")
    gentv_gfp_cneg("gfp_cneg_cc.tv")
