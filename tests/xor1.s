.name "xor_REG_REG"
.comment "Verify the functionality of the XOR instruction where the operands are register and a register"

	ld %16909060,r2
	xor r1,r2,r3
	st r3,7
