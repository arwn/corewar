.name "lldi_test1"
.comment "Verify the functionality of the LLDI instruction"

	lldi 600,%1,r02
	lldi %270544960,r02,r01
	lldi r01,r02,r03
	st r1,17                    ; 03 70 01 00 11
	st r2,17                    ; 03 70 02 00 11
	st r3,17                    ; 03 70 03 00 11
