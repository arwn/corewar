.name "Fuck you zork"
.comment "I print shit"

	st r01, r03
	ld %1, r04
	ld %100, r05
	ld %4, r06
	add r05, r06, r07
	st r01, r08
	st r01, r09
	st r01, r10
	st r01, r11
	st r01, r12
	st r01, r13
	st r01, r14
	st r01, r15
	st r01, r16
	sti r04, %:live, r05
	sti r01, %:live, r07
	ld %70,r02
	aff r02
	ld %117,r02
	aff r02
	ld %99,r02
	aff r02
	ld %107,r02
	aff r02
	ld %32,r02
	aff r02
	ld %121,r02
	aff r02
	ld %111,r02
	aff r02
	ld %117,r02
	aff r02
	ld %44,r02
	aff r02
	ld %32,r02
	aff r02
	ld %122,r02
	aff r02
	ld %111,r02
	aff r02
	ld %114,r02
	aff r02
	ld %107,r02
	aff r02
	ld %33,r02
	aff r02

l2:		sti r1, %:live, %1
		and r2, %0, r2

live:	live %1
