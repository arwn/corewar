.name "and_REG_REG"
.comment "Verify the functionality of the AND instruction"

	ld %16909060,r2
	and r1,r2,r3
	st r3,7
