$NOMOD51

;~ Defines
$include "aducgd1def.asm"

hh	equ	10h			;address for storing the hours
mm equ	11h			;idem minutes
ss equ	12h			;idem seconds

;~ Inits
.org 0x0000

;~ EntryPoint
mov sp, #0x7F

lcall	tftinit
mov	  hh,#23
mov   mm,#58
mov	  ss,#56

loop:	
mov	  tftxpos,#0
mov	  tftypos,#0

mov	  a,hh     	
lcall disp_b8
mov	  a,#':'			    
lcall	tftoutchar

mov	  a,mm     	
lcall	disp_b8 	
mov	  a,#':'			    
lcall	tftoutchar

mov	  a,ss
lcall disp_b8

inc	 ss
mov	 a,ss        
cjne	a,#60,sec_inc 
mov	 ss,#00h

inc  mm
mov  a, mm
cjne a, #60, sec_inc
mov  mm, #00h

inc hh
mov  a, hh
cjne a, #24, sec_inc
mov  hh, #00h

sec_inc:

mov a, #20
lcall delaya0k05s

ljmp	loop

disp_b8:
lcall	hexbcd8 	
lcall	tftoutbyte

ret

;~ Libraries
$include "aducgd1inc.asm"

end
