.name "lldi champ"
.comment "same as ldi but better and I can modify the carry"

	lldi 600,%1,r02
	lldi %20,r02,r01
	lldi	r01,r02,r03
