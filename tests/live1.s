.name "live_test1"
.comment "Verify the functionality of the LIVE instruction"


	ld	%16909060,	r03	;    5 cycles
	lfork	%:blabbo		; 1000 cycles
	and	r01,	r02,	r08	;    6 cycles
	live	%-1			;   10 cycles
blabbo:	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles
	lldi	r02,	r02,	r02	;   50 cycles

	st	r01,	7		;    5 cycles
