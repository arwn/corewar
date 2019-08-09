.name "xor_DIR_DIR"
.comment "Verify the functionality of the XOR instruction where the operands are a direct value and a direct value"

	xor %-1,%65535,r6
	st r6,7
