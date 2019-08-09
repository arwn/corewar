.name "champtest"
.comment "load into register, register into memory"
	
	ld	%42, r2
	st	r2, 21
	ldi	r1, %84, r2
	sti	r2, r1, r2
	
