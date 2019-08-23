.name "lld_test2"
.comment "Verify the functionality of the LLD instruction"
; 35
lld %-559038737, r2 ; 10
zjmp %:carrycheck ; 20
st r2, 6 ; 5
aff r1
aff r1
aff r1
aff r1
carrycheck:
st r2, 6 ; 5
