;org 0000h

; p0 .equ 080H
; p2 .equ 0A0H

loop:
 ; mov 0xA0, 0x80
	;	mov p2, p0
 
 mov 0xA0, #0x00			
	
	ljmp loop
