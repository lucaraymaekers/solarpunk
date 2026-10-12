org 0000h

p0 data 080H
p2 data 0A0H

loop:
    ; mov 0xA0, 0x80
				mov p2, p0
				ljmp loop
