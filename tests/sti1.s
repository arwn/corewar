.name "sti_test1"
.comment "Verify the functionality of the STI instruction"

	ld %16909060,r02
	ld %270544960,r03
	ld 0,r04
	sti r02,%28,%7
	sti r03,%40,r04
	sti r04,2000,r02
