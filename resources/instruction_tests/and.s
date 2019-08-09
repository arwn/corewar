.name "and champ"
.comment "Verify the functionality of the AND instruction"

	ld %16909060,r2
	and r1,r2,r3
	and r1,%270544960,r4
	and r1,-8,r5
	and %-1,%65535,r6
	and %-1,-32,r7
	and -32,1,r8
	st r3,75
	st r4,134
	st r5,193
	st r6,252
	st r7,311
	st r8,370
;	ld %14,r02
;	ld %0,r01
;	ld 42,r03
;	and r1,r2,r3
;	and %1,10,r2
;	and %1,r2,r4
