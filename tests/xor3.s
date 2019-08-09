.name "xor_REG_IND"
.comment "Verify the functionality of the XOR instruction where the operands are a register and an indirect value"

	xor r1,-8,r5
	st r5,7
