.name "or_DIR_DIR"
.comment "Verify the functionality of the OR instruction"

	or %-1,%65535,r6
	st r6,7
