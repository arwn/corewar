.name "ldi champ"
.comment "I can add most things and put it into  registry"

	ldi 20,%1,r02		; 0a e4 00 14 00 01 02
	ldi %20,r02,r01		; 0a 94 00 14 02 01
	ldi r01,r02,r03		; 0a 54 01 02 03
	st r1,17			; 03 70 01 00 11
	st r2,17			; 03 70 02 00 11
	st r3,17			; 03 70 03 00 11
