.name "my boi"
.comment "yee"

start:
	st r1, 6
	live %0
	fork %:b

a:							# a - payload = 122
	fork %:ab

aa:						# aa - ab = 29
	fork %:aab				# 3

aaa:						# aaa - aab = 13
							# aaa - payload = 116
	ld 116, r5				# 5
	and r2, r2, r2			# 5
	zjmp %:payload			# 3

aab:
	ld 103, r5
	and r2, r2, r2
	zjmp %:payload

ab:
	fork %:aad

aac:
	ld 87, r5
	and r2, r2, r2
	zjmp %:payload

aad:
	ld 74, r5
	and r2, r2, r2
	zjmp %:payload

b:							# b - payload = 61
	fork %:bb

ba:
	fork %:bab

baa:
	ld 55, r5
	and r2, r2, r2
	zjmp %:payload

bab:
	ld 42, r5
	and r2, r2, r2
	zjmp %:payload

bb:
	fork %:bad

bac:
	ld 26, r5
	and r2, r2, r2
	zjmp %:payload

bad:
	ld 13, r5
	and r2, r2, r2
	zjmp %:payload

payload:
	st r1, 6
	live %0
	st r5, 40
	fork %-20
