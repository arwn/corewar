.name "lfork_test1"
.comment "Verify basic functionality of LFORK instruction"

	ld	%16909060, r03		;    5 cycles
	ld	%270544960, r04		;    5 cycles
	lfork	%:label			; 1000 cycles
	st	r03, 7			;    5 cycles
	aff	r99			;    2 cycles
	aff	r99			;    2 cycles
	zjmp	%50			;   20 cycles
label:	st	r04, 7			;    5 cycles


	; lfork %600
