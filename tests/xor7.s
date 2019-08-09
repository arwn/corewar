.name "xor_IND_DIR"
.comment "Verify the functionality of the XOR instruction where the operands are an indirect value and a direct value"

	xor 3,%-1,r8
	st r8,7
