; p2 .equ 0xA0

;        .module main
;        .area CSEG (CODE)

;        ljmp start            ; Reset entry at address 0000h

org 0000h

start:
        mov  0xA0, #0xAA      	; Turn LEDs on
        ljmp start             ; Repeat forever