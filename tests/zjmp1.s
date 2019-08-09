.name "zjmp_test1"
.comment "Verify the functionality of the ZJMP instruction"

	and	r1,	r2,	r3	;  6
	zjmp	%:skip			; 20
neg:
	st	r1,	5		;  5
skip:
	zjmp	%:neg			; 20
