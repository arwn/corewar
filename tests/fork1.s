.name "fork_test1"
.comment "Verify basic functionality of FORK instruction"

	ld	%16909060, r03		;   5 cycles
	ld	%270544960, r04		;   5 cycles
	live	%0		;fakc
	fork	%:label			; 800 cycles
	st	r03, 7			;   5 cycles
	aff	r99			;   2 cycles
	aff	r99			;   2 cycles
	and	r01, r02, r05		;   6 cycles
	zjmp	%50			;  20 cycles
label:	st	r04, 7			;   5 cycles



; 	ld	%1, r16
; 	st	r1, 6
; lv:	live	%0
; 	zjmp	%:lv
; 	st	r15, -12
; 	st	r16, -22
; hh:	fork	%:jj
; 	fork	%0
; 	sub	r2, r1, r3
; 	sti	r3, r2, %12
; 	and	r1, r9, r3
; 	zjmp	%:hh
