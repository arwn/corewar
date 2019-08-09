.name "sti champ"
.comment "copy whatever is the registry (first param) into the result of the addition of the second and third params"

	ld %14,r02
	ld %0,r01
	ld 42,r03
	sti r02,%1,%15
	sti r03,%2,r01
	sti r01,42,r02
	st r1,17
	st r2,17
	st r3,17
