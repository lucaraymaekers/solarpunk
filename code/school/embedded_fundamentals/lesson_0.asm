$NOMOD51

;~ Includes
$include "aducgd1.asm"

  ;~ Inits
  .org 0x0000
  mov sp, 0x7F
  
  ;~ EntryPoint
  loop:

mov p2, p0

 ljmp loop

end