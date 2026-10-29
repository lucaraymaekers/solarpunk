$NOMOD51

;~ Defines
$include "aducgd1def.asm"

;~ Inits
.org 0x0000

;~ EntryPoint
mov sp, #0x7F

lcall tftinit
mov   tftxpos, #1
mov   tftypos, #1
mov   dptr, #namex

mov red, #0b11111
mov green, #0b111111
mov blue, #0b00000
lcall tftsetfrcolor

mov red, #0b11111
mov green, #0b000000
mov blue, #0b11111
lcall tftsetbgcolor

lcall tftoutmsga

loop: 
ljmp loop

namex:		
db	"Luca", 0

;~ Libraries
$include "aducgd1inc.asm"

end
