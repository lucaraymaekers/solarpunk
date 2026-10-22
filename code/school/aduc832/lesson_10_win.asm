$NOMOD51

; Template exersise 10 (runs on ADuC832 V3/4 board).
; Usage of the display functions and arithmetic.
; Show 2 8-bit counters (1 counts up by one, the other counts down by one).
; Print the counter values in BCD and HEX on the display (4 different locations).
;
; This program will show wrong results. Why?
; Avoid this by limiting the counters (see limitations of HEXBCD8).
;
; Remedy this by using HEXBCD16
;
; This template is used for the 3 tasks.
;
; Written by Roggemans M. (MGM) v2 on 02-02-2016
; Revisited by MGM gd on 07-2022

.org 0x0000
mov sp, 0x7F;

lcall tftinit 		 ;initialization of the TFT display
mov	  r0, #000h		;Used for counting up
; 63h = 99
mov	  r1, #063h		;used for counting down 

lus:	
mov	  tftxpos, #0		;cursor routing (horizontal)
mov	  tftypos, #0		;cursor routing (vertical)
mov	  a, r0			     ;Printing contents of r0 can only be done by storing them in a
lcall tftoutbyte	 	;number in a register is printed in hex representation

mov	  tftxpos,#10		      ;cursor routing
mov	  a,r0		       	     ;value stored in r0 will be translated into BCD
lcall print

;idem hex for r1 on second line
mov   tftypos, #1
mov   a, r1
lcall print

inc	  r0
cjne  r0, #100, increment
mov   r0, #0
increment:

dec   r1
cjne  r1, #0FFh, decrement
mov   r1, #63h
decrement:

ljmp  lus			;infinite loop

; input: a
print:
push  acc
mov   tftxpos, #0
lcall tftoutbyte
mov	  tftxpos, #10		
lcall hexbcd8
lcall tftoutbyte
pop   acc
ret

#include "aducgd1.asm"