.name "add_test1"
.comment "Verify functionality of ADD"

	ld	%-559087616,	r03	; 5
	ld	%48879,	r04	; 5
	add	r03,	r04,	r02	; 6
	st	r02,	6		; 5
