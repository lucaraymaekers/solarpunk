.area HOME (CODE)
.area CSEG (CODE)

ljmp loop

loop:
 mov 0xA0, #0x80
	
	ljmp loop
