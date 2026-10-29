$NOMOD51

;~ Defines
$include "aducgd1def.asm"

;~ Inits
.org 0x0000

;~ EntryPoint
mov sp, #0x7F

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

;~ Libraries
$include "aducgd1inc.asm"

end
