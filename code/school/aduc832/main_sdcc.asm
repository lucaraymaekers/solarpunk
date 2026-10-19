$NOMOD51

.org 0x0000

loop:
 mov p2, p0

 mov a, #10
 lcall delaya0k05s

	mov a, p0
 xrl a, #0xFF

	mov p2, a

 mov a, #10
	lcall delaya0k05s

	ljmp loop

#include "aducgd1.inc"
