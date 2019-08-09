.name "lld_test1"
.comment "Verify the functionality of the LLD instruction"

	lld %16909060,r02    ;
	lld %270544960,r01   ;
	lld 5,r03            ;
	st r1,17            ; 03 70 01 00 11
	st r2,17            ; 03 70 02 00 11
	st r3,17            ; 03 70 03 00 11
