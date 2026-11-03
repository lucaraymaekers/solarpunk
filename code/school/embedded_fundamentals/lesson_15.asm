$NOMOD51

;~ Defines
$include "aducgd1def.asm"

;~ Inits
.org 0x0000

;~ EntryPoint
mov sp, #0x7F
mov p2, #0x00

lcall tftinit

loop: 
;- Debugging 
; TODO(luca): $if 0
mov tftxpos, #0
mov a, p0
lcall tftoutbyte

mov tftxpos, #3
mov a, r0
lcall tftoutbyte
; TODO(luca): $end
;-

; R0 has mask
; start is for P2.7
mov r0, #0b01111111
; B has pattern count
mov b, #4

check_pressed:
; Check bit
mov a, p0
xrl a, r0
jz pressed

; Rotate r0
mov a, r0
rr  a
mov r0, a

djnz b, check_pressed

; Nothing was pressed
ljmp loop

pressed:
; Debounce delay
mov   a, #4
lcall delaya0k05s

; Wait for depress 
wait:     ; ex. 
mov a, p0 ; 0111 1111
xrl a, r0 ; 1000 0000 <- mask for pressed button 
jz wait 

; B holds our pattern value
mov a, b

light0:
cjne a, #4, light1
mov p2, #0b01111110
ljmp light_end

light1:
cjne a, #3, light2
mov p2, #0b10111101
ljmp light_end

light2:
cjne a, #2, light3
mov p2, #0b11011011
ljmp light_end

light3:
cjne a, #1, light4
mov p2, #0b11100111
ljmp light_end

light4:
; Unhandled pattern
ljmp loop

light_end:

; 9: Rotate 8 times and then once more to get back in starting pos
mov b, #9
mov a, p2
loop_light:

mov p2, a
rl  a

push acc
mov a, #2
lcall delaya0k05s
pop acc

djnz b, loop_light

ljmp loop

;~ Libraries
$include "aducgd1inc.asm"

end
