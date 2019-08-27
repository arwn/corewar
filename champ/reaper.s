.name "The Reaper"
.comment "OOOOOO Spooky Scary Ghosty"
start:
    fork %:round2
round1:
    xor r1, %1, r2
    st r2, 6
    live %0
    fork %:r1p2
r1p1:
    xor r2, %2, r2
r1p1jmp:
    st r2, 6
    kill %0
    live %0
    and r3, r4, r5
    zjmp %:r1p1jmp
r1p2:
    st r2, 6
    kill %0
    live %0
    and r3, r4, r5
    zjmp %:r1p2
round2:
    st r2, 6
    live %0
    fork %:r2p2
r2p1:
    xor r1, %2, r2
r2p1jmp:
    st r2, 6
    kill %0
    live %0
    and r3, r4, r5
    zjmp %:r2p1jmp
r2p2:
    st r1, 6
    live %0
    and r3, r4, r5
    zjmp %:r2p2
