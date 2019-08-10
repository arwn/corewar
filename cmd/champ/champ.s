.name "my boi"
.comment "yee"

start:
	st r1, 6
	live %0
	fork %:b

a:							# a - payload = 138
	fork %:ab

aa:						# aa - ab = 33
	fork %:aab				# 3

aaa:						# aaa - aab = 15
	ld 132, r5				# 5
	# and r2, r2, r2		# 5
	or 44, 0, r4			# 7
	zjmp %:payload			# 3

aab:
	ld 136, r5
	# and r2, r2, r2
	or 48, 0, r4
	zjmp %:payload

ab:
	fork %:abb

aba:
	ld 140, r5
	# and r2, r2, r2
	or 52, 0, r4
	zjmp %:payload

abb:
	ld 144, r5
	# and r2, r2, r2
	or 56, 0, r4
	zjmp %:payload

b:							# b - payload = 69
	fork %:bb

ba:
	fork %:bab

baa:
	ld 148, r5
	# and r2, r2, r2
	or 60, 0, r4
	zjmp %:payload

bab:
	ld 152, r5
	# and r2, r2, r2
	or 64, 0, r4
	zjmp %:payload

bb:
	fork %:bbb

bba:
	ld 156, r5
	# and r2, r2, r2
	or 68, 0, r4
	zjmp %:payload

bbb:
	ld 160, r5
	# and r2, r2, r2
	or 72, 0, r4
	zjmp %:payload

payload:
	st r1, 6
	live %69
	st r5, r6
	fork %-20
