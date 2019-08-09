.name "add champ"
.comment "add registries and put result in a registry"

	ld %16909060,r03
	ld %270544960,r04
	add r03,r04,r02
	st r02,6
;	add r1,r2,r3
;	ld %14,r02
;	ld %0,r01
;	ld 42,r03
;	add r1,r2,r3
;	add r3,r2,r1
;	add r1,r3,r3
