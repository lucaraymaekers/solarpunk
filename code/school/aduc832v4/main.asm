        .module main
        .area CSEG (CODE)

        ljmp start            ; Reset entry at address 0000h

start:
        mov 0xA0, #0xA0      	; Turn LEDs on
        ljmp start            ; Repeat forever