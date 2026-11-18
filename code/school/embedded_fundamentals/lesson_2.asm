;-
; Sample to learn how to use the LCD
;-
; Resources:
; - ILI9341 Driver User Manual:
;   https://www.lcdwiki.com/res/MSP2202/ILI9341_Datasheet.pdf
;-

$NOMOD51

;~ Defines
$include "aducgd1def.asm"

tile_size equ 2
x     equ 0x10
y     equ 0x11
dir_x equ 0x12
dir_y equ 0x13
temp  equ 0x14
inp   equ 0x15

;~ Inits
.org 0x0000

;~ EntryPoint
mov sp, #0x7F

; Save pllcon for debugging
mov 10h, pllcon

lcall tft_init
mov x, #160
mov y, #120
mov dir_x, #1
mov dir_y, #0

mov inp, p0

loop: 

; Erase snake
mov r5, #0x00
mov r6, #0x00
lcall draw_snake

;- Input handling 

; Left pressed  = rotate -clockwise
; Right pressed = rotate  clockwise

; N = dir_y  1
; E = dir_x  1
; S = dir_y -1 
; W = dir_x -1

; Detect the transition, not the state, detect key press

mov a, inp
xrl a, p0
cjne a, #0, pressed
ljmp nothing_pressed

pressed:
mov inp, p0

jnb p0.7, left_pressed
jnb p0.4, right_pressed
ljmp nothing_pressed

right_pressed:

mov a, dir_x 
cjne a, #0, y_set

clr c
mov a, dir_x
subb a, dir_y
mov dir_x, a
mov dir_y, #0

ljmp nothing_pressed

left_pressed:

mov a, dir_y 
cjne a, #0, y_set

rotate:

clr c
mov a, dir_y
subb a, dir_x
mov dir_y, a
mov dir_x, #0

ljmp nothing_pressed

y_set:

mov a, dir_x
mov dir_x, dir_y
mov dir_y, a

nothing_pressed:

;- Update position
mov a, x
add a, dir_x
mov x, a
mov a, y
add a, dir_y
mov y, a

mov a, x

cjne a, #20, not_reset_x_min
mov x, #199
not_reset_x_min:

cjne a, #200, not_reset_x_max
mov x, #21
not_reset_x_max:

mov a, y

cjne a, #20, not_reset_y_min
mov y, #159
not_reset_y_min:

cjne a, #160, not_reset_y_max
mov y, #21
not_reset_y_max:
;-

; Draw snake 
mov r5, #0xFF
mov r6, #0xFF
lcall draw_snake

mov a, #1
;lcall delaya0k05s
lcall some_delay
lcall some_delay
lcall some_delay
lcall some_delay

ljmp loop

;~ Libraries

some_delay:
mov b, #0xFF
some_delay_loop:
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
djnz b, some_delay_loop
ret

;- Draw snake 
draw_snake:
push acc

;- Set window 
mov	a,#2ah		      ;command: set column address
lcall	tftwrcbyte
mov	a,#0	         ;xstart h
lcall	tftwrdbyte
mov	a,x           ;xstart l
lcall	tftwrdbyte	 
mov	a,#0       	 	;xend h
lcall	tftwrdbyte
mov	a,x           ;xend l
add a, #tile_size
lcall	tftwrdbyte	

mov	a,#2bh		      ;command: set page line
lcall	tftwrcbyte
;                  High is 0 because max y is 240 < 256.
mov	a,#00h	     	 ;ystart h
lcall	tftwrdbyte
mov	a,y           ;ystart l
lcall	tftwrdbyte	
mov	a,#00h		      ;yend h
lcall	tftwrdbyte
mov	a,y           ;yend l
add a, #tile_size
lcall	tftwrdbyte

;- Draw 
mov	a,#2ch        ;command: memory write RAMWR
lcall	tftwrcbyte

draw_tile:

mov	a, r5		       ; color h
lcall	tftwrdbyte
mov	a, r6 	       ; color l
lcall	tftwrdbyte


pop acc

ret
;-

tft_init:
push	psw			;BACK UP REGISTERS ONTO STACK
push	acc			;back up accu
push	b			  ;needed for loop counting
push	00h			;pointer is needed for loading memory f0h-ffh

mov	pllcon,#00h		;set system clock to 16.777216 MHz
; on power on the tft module needs to be reset with a pulse >10us.
setb	cs			;disable display module
clr	reset			;hardware reset display module
mov	a,#100			;accu used as loopcouter (should also work on ADuC841)
djnz	acc,$			;repeat (total time should be >10us)
setb	reset			;end hardware reset of display module
mov	a,#3			  ;after reset 120ms delay is needed
lcall	delaya0k05s
; The microcontroller SPI interface needs to be enabled and configured

mov	spicon,#00110001b
; 0: Interrupt bit cleared
; 0: clear collision error
; 1: enable SPI
; 1: Enable master mode
; 0: clock idles low
; 0: trailing (rising) edge clocks data in slave
; 01: fosc/4

; use xxxx shift clock.
; The display module allows 10MHz SPI write clock, but the SPI read clock cannot exceed 6.6MHz.
; Experiments show the shift clock is dependend on PLL settings. The display operates with fosc/4
; with system clock 16.777216 MHz. The level shifters used on the ADuCV3 board limit shift clock
; to this value.....Sorry...

; Default settings of the ILI9341:

mov	a,#11h			     ;sleep out command
lcall	tftwrcbyte

; This command demands a delay of minimal 120ms. Required by sleep out command

mov	a,#3	       		;delay 50ms used 3 times
lcall	delaya0k05s

mov	a,#29h		    	 ;display on command
lcall	tftwrcbyte

mov	a,#3ah		     	;pixel format set (number of bits/pixel)
lcall	tftwrcbyte		
mov	a,#01010101b
; 0:   defined
; TODO(luca): Check if DPI format unused since we are using SPI only MCU should matter.
; 101: 16bit RGB display interface format
; 0:   defined
; 101: 16bit MCU interface format
lcall	tftwrdbyte

mov	a,#36h			     ;memory access control (MADCTL)
lcall	tftwrcbyte

mov	a,#11101000b		
; 1: my  writing left to right
; 1: mx  writing top to down
; 1: mv  row and column are excanged
; 0: ml  vertical refresh top to down
; 1: bgr set to rgb
; 0: mh  horizontal refresh left to right
; 00: defined
lcall	tftwrdbyte		;send as parameter

mov	red,#00h		;red color minimal
mov	green,#00h		;green color minimal
mov	blue,#00h		;blue color minimal
lcall	tftsetbgcolor		;write colors as 16 bit value in bgcolor registers

mov	red,#00;011111b	;red color maximum
mov	green,#00111111b	;green color maximum
mov	blue,#0;0011111b	;blue color maximum
lcall	tftsetfrcolor		;write colors as 16 bit value in bgcolor registers

lcall	tftclear		;clear=fill all pixels with background color		

pop	00h			;restoring used registers	
pop	b
pop	acc
pop	psw
ret

$include "aducgd1inc.asm"

end
