$NOMOD51

        ;~ Defines
$include "aducgd1def.asm"

;~ Inits
.org 0x0000

  ;~ EntryPoint
mov sp, #0x7F
mov p2, #0xAA

loop: 

ljmp loop

;~ Libraries
$include "aducgd1inc.asm"
        
        end
  