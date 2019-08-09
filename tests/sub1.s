.name "sub_test1"
.comment "Verify the functionality of the SUB instruction"

	st r01,r02
	ld %16909060,r03
	sub r02,r03,r04
	st r04,6
;	sub r1,r2,r3
;	ld %14,r02
;	ld %0,r01
;	ld 42,r03
;	sub r1,r2,r3
;	sub r3,r2,r1
;	sub r1,r3,r3
