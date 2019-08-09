.name "xor_IND_REG"
.comment "Verify the functionality of the XOR instruction where the operands are an indirect value and a register"

	xor -8,r1,r5
	st r5,7
