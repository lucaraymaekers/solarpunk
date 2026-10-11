

p2 .equ 0xA0

        .module main
        .area CSEG (CODE)

        ljmp start            ; Reset entry at address 0000h

start:
        mov 0xp2, #0xAA      	; Turn LEDs on
        ljmp start            ; Repeat forever