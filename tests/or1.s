.name "or_REG_REG"
.comment "Verify the functionality of the OR instruction"

	ld %16909060,r2
	or r1,r2,r3
	st r3,7
