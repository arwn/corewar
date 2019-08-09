.name "zjmp champ"
.comment "I only jump under certain conditions. Don't tell me what to do!"

	and r1,r2,r3
	zjmp %10
	and r1,r2,r3
	zjmp %255
	and r1,r2,r3
	zjmp %10
