###############################################################################
#### Generation of Pseudo-Random Test-Vectors (PRTV) for GF(p) Arithmetic #####
###############################################################################

# Define the prime p
p = 2**255 - 19

# Define "pseudo-random" start operands
op1 = 0x0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
op2 = 0x76543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA98


def gentv_gfp_add(tvfilename, numtv):
    res1 = op1
    res2 = op2
    # Open the output file
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Pseudo-Random Test-Vectors (PRTV) for Modular Addition\n")
        for idx in range (0, (numtv//2)):
            tvfile.write(f"op1: 0x{res1:064X}\n")
            tvfile.write(f"op2: 0x{res2:064X}\n")
            res1 = (res1 + res2) % p
            tvfile.write(f"res: 0x{res1:064X}\n")
            tvfile.write(f"op1: 0x{res2:064X}\n")
            tvfile.write(f"op2: 0x{res1:064X}\n")
            res2 = (res2 + res1) % p
            tvfile.write(f"res: 0x{res2:064X}\n")
        tvfile.close()
    print(f"{numtv} pseudo-random test-vectors written to {tvfilename}")


def gentv_gfp_sub(tvfilename, numtv):
    res1 = op1
    res2 = op2
    # Open the output file
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Pseudo-Random Test-Vectors (PRTV) for Modular Subtraction\n")
        for idx in range (0, (numtv//2)):
            tvfile.write(f"op1: 0x{res1:064X}\n")
            tvfile.write(f"op2: 0x{res2:064X}\n")
            res1 = (res1 - res2) % p
            tvfile.write(f"res: 0x{res1:064X}\n")
            tvfile.write(f"op1: 0x{res2:064X}\n")
            tvfile.write(f"op2: 0x{res1:064X}\n")
            res2 = (res2 - res1) % p
            tvfile.write(f"res: 0x{res2:064X}\n")
        tvfile.close()
    print(f"{numtv} pseudo-random test-vectors written to {tvfilename}")


def gentv_gfp_mul(tvfilename, numtv):
    res1 = op1
    res2 = op2
    # Open the output file
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Pseudo-Random Test-Vectors (PRTV) for Modular Multiplication\n")
        for idx in range (0, (numtv//2)):
            tvfile.write(f"op1: 0x{res1:064X}\n")
            tvfile.write(f"op2: 0x{res2:064X}\n")
            res1 = (res1 * res2) % p
            tvfile.write(f"res: 0x{res1:064X}\n")
            tvfile.write(f"op1: 0x{res2:064X}\n")
            tvfile.write(f"op2: 0x{res1:064X}\n")
            res2 = (res2 * res1) % p
            tvfile.write(f"res: 0x{res2:064X}\n")
        tvfile.close()
    print(f"{numtv} pseudo-random test-vectors written to {tvfilename}")


def gentv_gfp_mul32(tvfilename, numtv):
    res1 = op1
    # Open the output file
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Pseudo-Random Test-Vectors (PRTV) for Modular Multiplication (32 bit)\n")
        for idx in range (0, numtv):
            tvfile.write(f"op1: 0x{res1:064X}\n")
            res1 = (res1 * 121666) % p
            tvfile.write(f"res: 0x{res1:064X}\n")
        tvfile.close()
    print(f"{numtv} pseudo-random test-vectors written to {tvfilename}")


def gentv_gfp_sqr(tvfilename, numtv):
    res1 = op1
    # Open the output file
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Pseudo-Random Test-Vectors (PRTV) for Modular Squaring\n")
        for idx in range (0, numtv):
            tvfile.write(f"op1: 0x{res1:064X}\n")
            res1 = (res1 * res1) % p
            tvfile.write(f"res: 0x{res1:064X}\n")
        tvfile.close()
    print(f"{numtv} pseudo-random test-vectors written to {tvfilename}")


def gentv_gfp_hlv(tvfilename, numtv):
    res1 = op1
    # Open the output file
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Pseudo-Random Test-Vectors (PRTV) for Modular Halving\n")
        for idx in range (0, numtv):
            tvfile.write(f"op1: 0x{res1:064X}\n")
            res1 = (res1 * pow(2, -1, p)) % p
            tvfile.write(f"res: 0x{res1:064X}\n")
        tvfile.close()
    print(f"{numtv} pseudo-random test-vectors written to {tvfilename}")


def gentv_gfp_cneg(tvfilename, numtv):
    res1 = op1
    # Open the output file
    with open(tvfilename, "w") as tvfile:
        tvfile.write("# Pseudo-Random Test-Vectors (PRTV) for Conditional Modular Negation\n")
        for idx in range (0, numtv):
            tvfile.write(f"op1: 0x{res1:064X}\n")
            if (idx % 2 == 1): res1 = p - res1
            tvfile.write(f"res: 0x{res1:064X}\n")
            if (idx % 2 == 1): res1 = (res1 * res1) % p
        tvfile.close()
    print(f"{numtv} pseudo-random test-vectors written to {tvfilename}")


if __name__ == "__main__":
    gentv_gfp_add("gfp_add_pr.tv", 1000)
    gentv_gfp_sub("gfp_sub_pr.tv", 1000)
    gentv_gfp_mul("gfp_mul_pr.tv", 1000)
    gentv_gfp_mul32("gfp_mul32_pr.tv", 1000)
    gentv_gfp_sqr("gfp_sqr_pr.tv", 1000)
    gentv_gfp_hlv("gfp_hlv_pr.tv", 1000)
    gentv_gfp_cneg("gfp_cneg_pr.tv", 1000)
