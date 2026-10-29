$NOMOD51

;~ Defines
$include "aducgd1def.asm"

count equ 10h

;~ Inits
.org 0x0000

;~ EntryPoint
mov sp, #0x7F

mov count, #0
lcall tftinit

loop: 

mov   tftxpos, #0
mov   a, count
lcall hexbcd8
lcall tftoutbyte

wait_pressed: 
jb    p0.7, wait_pressed

; debouncing delay
mov   a, #4
lcall delaya0k05s

wait_depressed:
jnb   p0.7, wait_depressed

inc   count

ljmp loop

;~ Libraries
$include "aducgd1inc.asm"

end
