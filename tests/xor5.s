.name "xor_DIR_IND"
.comment "Verify the functionality of the XOR instruction where the operands are a direct value and an indirect value"

	xor %-1,-32,r7
	st r7,7
