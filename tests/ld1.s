.name "ld_test"
.comment "Verify the functionality of the LD instruction"

	ld %16909060,r02    ;
	ld %270544960,r01   ;
	ld 5,r03            ;
	st r1,17            ; 03 70 01 00 11
	st r2,17            ; 03 70 02 00 11
	st r3,17            ; 03 70 03 00 11
